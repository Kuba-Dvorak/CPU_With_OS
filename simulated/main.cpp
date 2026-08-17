// ============================================================================
//  Simulator CPU
//
//  ARCHITEKTURA
//  ------------
//  Vsechny prenosy i vsechny operace ALU jdou pres dva MOSTNI registry.
//  Zadny registr neni pripojeny na zadny jiny - kazdy je pripojeny jen
//  na mosty. Tim odpada matice kazdy-s-kazdym i multiplexery pred ALU.
//
//  REGISTRY PRISTUPNE Z KODU  (tyhle tri z realnych tranzistoru)
//      reg0 / reg0Hi     kopie vysledku ALU pro program - JEN CTENI
//      reg1              uzivatelsky, cteni i zapis
//      reg2              uzivatelsky, cteni i zapis
//
//  MOSTY - jedina cesta mezi cimkoliv, zaroven vstupy scitacky
//      busA / busAHi     vstup A scitacky + prenosova cesta
//      busB / busBHi     vstup B scitacky
//
//  INTERNI
//      aluOut / aluOutHi realny vystup scitacky. Pada sem KAZDA operace.
//                        Vystup jde na adresni piny RAM, takze spocitanou
//                        adresu lze hned pouzit.
//                        POZOR: tohle NENI instructionPointer. Dva ruzne
//                        registry, ktere spolu nemaji nic spolecneho.
//      instructionPointer  samostatny 16bit binarni CITAC, adresa instrukce.
//                          Neprochazi ALU, inkrementuje se hodinovou hranou.
//      op1..op4          operandy nactene instrukce
//      opcode            instruction register
//
//  Format instrukce: 5 bajtu = [opkod][op1][op2][op3][op4]
//  Adresy v operandech: vzdy DOLNI bajt prvni, potom horni.
// ============================================================================

#include <iostream>
#include <cstdint>
#include <cstring>

// ---------------------------------------------------------------------------
//  Registry
// ---------------------------------------------------------------------------

uint8_t reg0   = 0,  reg0Hi   = 0;    // kopie vysledku ALU, jen cteni
uint8_t reg1   = 0;                   // uzivatelsky
uint8_t reg2   = 0;                   // uzivatelsky

uint8_t busA   = 0,  busAHi   = 0;    // most A = vstup A scitacky
uint8_t busB   = 0,  busBHi   = 0;    // most B = vstup B scitacky

uint8_t aluOut = 0,  aluOutHi = 0;    // realny vystup ALU + adresa pro RAM

uint8_t op1 = 0, op2 = 0, op3 = 0, op4 = 0;
uint8_t opcode = 0;

uint16_t instructionPointer = 0;      // samostatny citac, NENI aluOut

uint8_t carryIn     = 0;              // u odcitani = 1
uint8_t carryOut    = 0;
uint8_t compareFlag = 0;

// ridici bit z mikrokodu: zkopirovat vysledek i do reg0/reg0Hi?
// false = interni vypocet (adresa), program o nem nevi
bool aluUserVisible = true;

uint8_t ram[65536];

// ---------------------------------------------------------------------------
//  ALU
//
//  Vstupy nejsou parametry - scitacka je natvrdo dratena na mosty.
//  Odcitani = dvojkovy doplnek: XOR hradla MEZI mostem B a scitackou
//  invertuji vstup, carry-in = 1. Tataz scitacka, zadny druhy obvod.
//  Obsah busB se pritom nemeni - hradla sedi az za registrem.
// ---------------------------------------------------------------------------

enum AluOp {
    ALU_ADD8, ALU_ADD16,
    ALU_SUB8, ALU_SUB16,
    ALU_NOT,  ALU_NOT16,
    ALU_SHL,  ALU_SHR,
    ALU_AND,  ALU_OR,
    ALU_CMP_GT, ALU_CMP_LT, ALU_CMP_EQ
};

void aluStore(uint8_t lo, uint8_t hi) {
    aluOut   = lo;
    aluOutHi = hi;
    if (aluUserVisible) {
        busA = aluOut;
        busAHi = aluOut;
        reg0 = busA;
        reg0Hi = busAHi;
    }
}

void addCore(bool wide) {
    uint16_t lo = (uint16_t)busA + (uint16_t)busB + carryIn;
    carryOut = (lo >> 8) & 1;
    if (!wide) {
        aluStore((uint8_t)lo, 0);
        return;
    }
    uint16_t hi = (uint16_t)busAHi + (uint16_t)busBHi + carryOut;
    aluStore((uint8_t)lo, (uint8_t)hi);
    carryOut = (hi >> 8) & 1;
}

void alu(AluOp op) {
    switch (op) {

    case ALU_ADD8:  carryIn = 0; addCore(false); break;
    case ALU_ADD16: carryIn = 0; addCore(true); break;

    case ALU_SUB8:  carryIn = 1; alu(ALU_NOT); addCore(false); break;
    case ALU_SUB16: carryIn = 1; alu(ALU_NOT16); addCore(true); break;

    case ALU_NOT:   aluStore((uint8_t)~busA, 0); break;
    case ALU_NOT16: aluStore((uint8_t)~busA, (uint8_t)~busAHi); break;

    case ALU_SHL:   aluStore((uint8_t)(busA << busB), 0); break;
    case ALU_SHR:   aluStore((uint8_t)(busA >> busB), 0); break;
    case ALU_AND:   aluStore((uint8_t)(busA &  busB), 0); break;
    case ALU_OR:    aluStore((uint8_t)(busA |  busB), 0); break;

    // Porovnani funguje takhle: vezme to ALU out, a porovna se kazdy bit specialni pres bit zavazani
    case ALU_CMP_GT: {
        uint8_t nonzero = 0;
        for (int i = 7; i >= 0; i--) if (aluOut & (1 << i)) nonzero = 1;
        compareFlag = (nonzero && !((aluOut >> 7) & 1)) ? 1 : 0;
        break;
    }

    case ALU_CMP_LT:
        compareFlag = (aluOut >> 7) & 1;
        break;

    case ALU_CMP_EQ: {
        uint8_t nonzero = 0;
        for (int i = 7; i >= 0; i--) if (aluOut & (1 << i)) nonzero = 1;
        compareFlag = nonzero ? 0 : 1;            // IRL NOR strom
        break;
    }
    }
}

// ---------------------------------------------------------------------------
//  MOSTY a adresace registru
//
//  Index registru v operandu = 4 bity:
//     0 reg0     (RO)    1 reg1            2 reg2           3 busA
//     4 busAHi           5 busB            6 busBHi         7 reg0Hi   (RO)
//     8 aluOut   (RO)    9 aluOutHi (RO)  10 op1           11 op2
//    12 op3            13 op4            14 opcode        15 volne
// ---------------------------------------------------------------------------

enum RegIdx {
    R_REG0 = 0, R_REG1, R_REG2, R_BUSA, R_BUSAHI, R_BUSB, R_BUSBHI, R_REG0HI,
    R_ALUOUT, R_ALUOUTHI, R_OP1, R_OP2, R_OP3, R_OP4, R_OPCODE
};

uint8_t regRead(uint8_t idx) {
    switch (idx) {
        case R_REG0:     return reg0;      case R_REG0HI:   return reg0Hi;
        case R_REG1:     return reg1;      case R_REG2:     return reg2;
        case R_BUSA:     return busA;      case R_BUSAHI:   return busAHi;
        case R_BUSB:     return busB;      case R_BUSBHI:   return busBHi;
        case R_ALUOUT:   return aluOut;    case R_ALUOUTHI: return aluOutHi;
        case R_OP1:      return op1;       case R_OP2:      return op2;
        case R_OP3:      return op3;       case R_OP4:      return op4;
        case R_OPCODE:   return opcode;
    }
    fault("neznamy index registru pri cteni");
    return 0;
}

void regWrite(uint8_t idx, uint8_t v) {
    switch (idx) {
        // vystupy ALU jsou jen pro cteni
        case R_REG0: case R_REG0HI: case R_ALUOUT: case R_ALUOUTHI:
            fault("zapis do vystupu ALU - jen pro cteni"); return;
        case R_REG1:   reg1   = v; return;   case R_REG2:   reg2   = v; return;
        case R_BUSA:   busA   = v; return;   case R_BUSAHI: busAHi = v; return;
        case R_BUSB:   busB   = v; return;   case R_BUSBHI: busBHi = v; return;
        case R_OP1:    op1    = v; return;   case R_OP2:    op2    = v; return;
        case R_OP3:    op3    = v; return;   case R_OP4:    op4    = v; return;
        case R_OPCODE: opcode = v; return;
    }
    fault("neznamy index registru pri zapisu");
}

void toBusA(uint8_t idx)   { busA = regRead(idx); }
void toBusB(uint8_t idx)   { busB = regRead(idx); }
void fromBusA(uint8_t idx) { regWrite(idx, busA); }

uint16_t makeAddr(uint8_t lo, uint8_t hi) {
    return (uint16_t)((uint16_t)hi << 8 | lo);
}

// ---------------------------------------------------------------------------
//  ROM - tabulka opkodu
//
//  00000 =  0  halt (a vsechno nezname / nenapsane)
//  00001 =  1  add8        op1=regA, op2=regB
//  00010 =  2  add16       op1=A_lo, op2=A_hi, op3=B_lo, op4=B_hi
//  00011 =  3  sub8        op1=regA, op2=regB
//  00100 =  4  moveRegReg  op1=zdroj, op2=cil
//  00101 =  5  moveRegRam  op1=reg,  op2=adr_lo, op3=adr_hi
//  00110 =  6  moveRamReg  op1=reg,  op2=adr_lo, op3=adr_hi
//  00111 =  7  moveRamRam  op1=src_lo, op2=src_hi, op3=dst_lo, op4=dst_hi
//  01000 =  8  jmp         op1=adr_lo, op2=adr_hi
//  01001 =  9  jmp >       op1=regA, op2=regB, op3=adr_lo, op4=adr_hi
//  01010 = 10  jmp <       dtto
//  01011 = 11  jmp =       dtto
//  01100 = 12  sub16       jako add16
//  01110 = 14  setReg      op1=reg, op2=hodnota
//  01111 = 15  setRam      op1=hodnota, op2=adr_lo, op3=adr_hi
//  10000 = 16  loadIdx     op1=cil, op2=baze_lo, op3=baze_hi, op4=reg s offsetem
//
//  IRL se to neifuje - opkod + cislo faze je adresa do ROM a obsah ROM
//  jsou primo ridici signaly. Tady je to switch jen kvuli citelnosti.
// ---------------------------------------------------------------------------

void compareRegs(uint8_t idxA, uint8_t idxB, AluOp cmp) {
    toBusA(idxA);
    toBusB(idxB);
    alu(ALU_SUB8);
    alu(cmp);
}

void operateOperation() {
    switch (opcode) {

    case 0:
        fault("halt");
        break;

    // --- aritmetika ------------------------------------------------------
    case 1:  toBusA(op1); toBusB(op2); alu(ALU_ADD8); break;   // add8
    case 3:  toBusA(op1); toBusB(op2); alu(ALU_SUB8); break;   // sub8

    case 2:  // add16
        toBusA(op1); busAHi = regRead(op2);
        toBusB(op3); busBHi = regRead(op4);
        alu(ALU_ADD16);
        break;

    case 12: // sub16
        toBusA(op1); busAHi = regRead(op2);
        toBusB(op3); busBHi = regRead(op4);
        alu(ALU_SUB16);
        break;

    // --- presuny (vzdy pres most A) --------------------------------------
    case 4:  toBusA(op1); fromBusA(op2); break;                // moveRegReg

    case 5:  // moveRegRam
        toBusA(op1);
        ram[makeAddr(op2, op3)] = busA;
        break;

    case 6:  // moveRamReg
        busA = ram[makeAddr(op2, op3)];
        fromBusA(op1);
        break;

    case 7:  // moveRamRam
        busA = ram[makeAddr(op1, op2)];        // faze 1: cteni -> most
        ram[makeAddr(op3, op4)] = busA;        // faze 2: most  -> zapis
        break;

    // --- skoky -----------------------------------------------------------
    case 8:
        instructionPointer = makeAddr(op1, op2);
        break;

    case 9:
        compareRegs(op1, op2, ALU_CMP_GT);
        if (compareFlag) instructionPointer = makeAddr(op3, op4);
        break;

    case 10:
        compareRegs(op1, op2, ALU_CMP_LT);
        if (compareFlag) instructionPointer = makeAddr(op3, op4);
        break;

    case 11:
        compareRegs(op1, op2, ALU_CMP_EQ);
        if (compareFlag) instructionPointer = makeAddr(op3, op4);
        break;

    // --- konstanty -------------------------------------------------------
    case 14: busA = op2; fromBusA(op1); break;                 // setReg
    case 15: ram[makeAddr(op2, op3)] = op1; break;             // setRam

    // --- indexovane cteni: reg <- RAM[baze + offset] ----------------------
    //  HOT PATH INTERPRETU: baze procesu + adresa z bajtkodu.
    //  Adresa se pocita do aluOut/aluOutHi a rovnou z nich jde na adresni
    //  piny RAM. Kopie do reg0 se nedela, takze vysledek uzivatelske
    //  instrukce zustane nedotceny.
    case 16:
        busA   = op2;                  // baze lo
        busAHi = op3;                  // baze hi
        toBusB(op4);                   // offset
        busBHi = 0;

        aluUserVisible = false;
        alu(ALU_ADD16);
        aluUserVisible = true;

        busA = ram[makeAddr(aluOut, aluOutHi)];
        fromBusA(op1);
        break;

    default:
        fault("neznamy opkod");
        break;
    }
}

// ---------------------------------------------------------------------------
//  Fetch. IP je samostatny binarni citac (74HC161 nebo retez T klopnych
//  obvodu) - inkrementovat ho pres ALU nejde, prepsal by vysledek
//  predchozi instrukce driv, nez ho stihne precist ta nasledujici.
// ---------------------------------------------------------------------------

void ipIncrement() { instructionPointer++; }

void mainLoop() {
    opcode = ram[instructionPointer];  ipIncrement();
    op1    = ram[instructionPointer];  ipIncrement();
    op2    = ram[instructionPointer];  ipIncrement();
    op3    = ram[instructionPointer];  ipIncrement();
    op4    = ram[instructionPointer];  ipIncrement();
    operateOperation();
}

void bootLoad() {
    // nechano na tebe - tady uz bude OS
}

// ---------------------------------------------------------------------------
//  Testy
// ---------------------------------------------------------------------------

int at = 0;
void emit(uint8_t op, uint8_t a = 0, uint8_t b = 0, uint8_t c = 0, uint8_t d = 0) {
    ram[at++] = op; ram[at++] = a; ram[at++] = b; ram[at++] = c; ram[at++] = d;
}

// Pocitej reg1 od 0 do 5.
// reg2 musi stridat dve role (krok a limit), protoze reg0 je jen pro cteni
// a plne pouzitelne registry jsou tedy jen dva.
void loadCounterTest() {
    at = 0;
    std::memset(ram, 0, sizeof(ram));

    emit(14, R_REG1, 0);                    //  0: reg1 = 0     citac
    emit(14, R_REG2, 1);                    //  5: reg2 = 1     krok
    emit( 1, R_REG1, R_REG2);               // 10: reg0 = reg1 + reg2
    emit( 4, R_REG0, R_REG1);               // 15: reg1 <- reg0
    emit(14, R_REG2, 5);                    // 20: reg2 = 5     limit
    emit(10, R_REG1, R_REG2, 5, 0);         // 25: if reg1 < reg2 -> 5
    emit( 0);                               // 30: halt
}

// Indexovane cteni nesmi zahodit vysledek v reg0.
void loadIndexTest() {
    at = 0;
    std::memset(ram, 0, sizeof(ram));
    ram[2005] = 77;

    emit(14, R_REG1, 5);                    //  0: reg1 = 5   offset
    emit(14, R_REG2, 3);                    //  5: reg2 = 3
    emit( 1, R_REG1, R_REG2);               // 10: reg0 = 8   <- musi prezit
    emit(16, R_REG2, 208, 7, R_REG1);       // 15: reg2 <- RAM[2000 + reg1]
    emit( 0);                               // 20: halt
}

void run(const char* name) {
    halted = false; haltReason = ""; instructionPointer = 0;
    std::cout << "=== " << name << " ===\n";
    int steps = 0;
    while (!halted && steps < 1000) {
        uint16_t before = instructionPointer;
        mainLoop();
        steps++;
        std::cout << "IP " << before << "\top " << (int)opcode
                  << "\treg0=" << (int)reg0
                  << " reg1=" << (int)reg1
                  << " reg2=" << (int)reg2
                  << " aluOut=" << (int)makeAddr(aluOut, aluOutHi)
                  << " cmp=" << (int)compareFlag << "\n";
    }
    std::cout << "zastaveno: " << haltReason << ", po " << steps << " instrukcich\n\n";
}

int main() {
    std::memset(ram, 0, sizeof(ram));
    bootLoad();

    loadCounterTest(); run("pocitadlo do 5");
    loadIndexTest();   run("indexovane cteni");
    return 0;
}