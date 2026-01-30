# MovFuscator 

## Membrii echipei

- **Chitic David-Alexandru, grupa 141**
- **Ivan Luca Cristian, grupa 141**
- **Cranta Eduard Alexandru, grupa 141**
- **Baicoianu Ștefan Tudor, grupa 141**

## Ce am făcut

Interpreter de cod Assembly x86 (AT&T) în C++ care simulează execuția instrucțiunilor și generează cod simplificat cu valori imediate calculate.

**Principiu:** Interpretat pas cu pas → registre, memorie, stivă simulate → operațiile evaluate → înlocuite cu rezultate concrete.

---

## Structura proiectului

### Cod sursă
- **`src/main.cpp`**

### Fișiere generate
- **`build/MovFuscator`** - executabil debug (pentru dezvoltare)
- **`out/release/MovFuscator`** - executabil release (cu optimizări de la compilator)

### Fișiere Assembly (input/output)
- **`out/release/asmFiles/`** - fișiere de intrare (ex1.s - ex17.s, main.s)
- **`out/release/asmOut/`** - fișiere de ieșire generate (create automat la rulare)

---

## Utilizare

```bash
cd out/release
./MovFuscator ex1.s ex2.s ex3.s
```

sau toate testele:
```bash
./MovFuscator ex{1..17}.s
```

**Output:** Fișiere în `asmOut/`

---

## Exemplu

**Input:**
```asm
mov $1, %eax
mov $2, %ebx
add %eax, %ebx  # calcul
```

**Output:**
```asm
movl $1, %eax
movl $2, %ebx
movl $3, %ebx  # 1+2 deja calculat
```

---

## Ce funcționează ✅

### Instrucțiuni suportate (implementate în main.cpp)

| Categorie | Instrucțiuni |
|-----------|--------------|
| **Mișcare date** | `mov/movl/movw/movb`, `lea/leal/leaw/leab` |
| **Aritmetice** | `add/addl/addw/addb`, `sub/subl/subw/subb`, `mul/mull/mulw/mulb`, `div/divl/divw/divb`, `inc/incl/incw/incb`, `dec/decl/decw/decb` |
| **Logice** | `and/andl/andw/andb`, `or/orl/orw/orb`, `xor/xorl/xorw/xorb` |
| **Shift** | `shl/shll/shlw/shlb`, `shr/shrl/shrw/shrb`, `sar/sarl/sarw/sarb` |
| **Comparare** | `cmp/cmpl/cmpw/cmpb`, `test/testl/testw/testb` |
| **Control flux** | `jmp`, `je/jz`, `jne/jnz`, `jl/jle`, `jg/jge`, `ja/jae`, `loop` |
| **Stivă** | `push/pushl/pushw/pushb`, `pop/popl/popw/popb` |
| **Funcții** | `call`, `ret` |
| **Altele** | `int $0x80` |

### Operanzi și secțiuni
- Registre: `%eax`, `%ebx`, `%ecx`, `%edx`, sub-registre: `%ax`, `%ah`, `%al`, etc.
- Valori: `$100`, `$0x1F`, `$0b1010`
- Adresare: `(%eax)`, `4(%ebx)`, `(%edi, %ecx, 4)`
- Secțiuni: `.data` (`.long`, `.word`, `.byte`), `.text`

---

## Limitări ⚠️

- Flaguri incomplete (doar E, L, G, LE, GE, Z, A, AE)
- Funcții externe (`printf`, `fflush`) doar copiate, nu executate
- Overflow/underflow negestionat

