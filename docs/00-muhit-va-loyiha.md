# 00. Muhit va loyiha: ikki kompyuterda bo'sh oyna

## Bu qadamda nima qo'shamiz va nega aynan hozir

Bu darsda OpenGL ham, suyuqlik ham yo'q. Natija bitta: siz yozgan kod Windows'da va Mac'da yig'iladi,
bo'sh oyna ochadi va GitHub orqali ikki kompyuter o'rtasida yuradi.

Kurs shundan boshlanishining sababi oddiy: keyingi darslarning hammasi shu zanjir ustida turadi.
Ekran qora chiqsa yoki zarrachalar portlab ketsa, sababni shader va fizikadan qidiramiz. Shu paytda
"compiler topilmadi" yoki "Mac'da boshqacha yig'ilyapti" degan muammolar aralashib ketmasligi kerak.

Dars oxirida loyiha shunday ko'rinadi:

```
opengl_fluid_sim_scratch/
├── .gitignore
├── CLAUDE.md          (Claude yozadi: kurs holati)
├── CMakeLists.txt
├── docs/              (Claude yozadi: darslar)
└── src/
    └── main.cpp
```

Avval to'rtta tushunchani ko'rib chiqamiz, keyin kodga o'tamiz.

## 1. Matndan ishlaydigan dasturgacha

Protsessor `main.cpp` matnini tushunmaydi, u faqat mashina kodini bajaradi. Matn mashina kodiga
uch bosqichda aylanadi:

```
main.cpp ──► preprocessor ──► compiler ──► main.cpp.obj ──┐
                                                          ├──► linker ──► fluid_sim.exe
                                     GLFW kutubxonasi ────┘
```

| Bosqich | Nima qiladi |
|---|---|
| Preprocessor | `#include`ni o'sha faylning matni bilan almashtiradi, `#define`larni qo'llaydi. Natija: bitta uzun matn. |
| Compiler | Shu matndan object file (`.obj` yoki `.o`) yasaydi. Har bir `.cpp` alohida kompilyatsiya qilinadi va boshqa `.cpp`lar haqida hech narsa bilmaydi. |
| Linker | Object file'lar va kutubxonalarni bitta dasturga ulaydi: `glfwInit` chaqirilgan joyni `glfwInit`ning haqiqiy kodi bilan bog'laydi. |

### Header va kutubxona: ikki xil narsa

Restoranni tasavvur qiling. Header (`glfw3.h`) bu **menyu**: qanday taomlar borligi va qanday buyurtma
berish yozilgan, ya'ni funksiya nomlari va parametr turlari. Kutubxona bu **oshxona**: taomni haqiqatan
tayyorlaydigan kod. Compiler faqat menyuga qaraydi: "bunday taom bormi, buyurtma to'g'rimi?". Oshxonani
linker qidiradi.

Shuning uchun xatolar ham ikki xil. Quyidagilarni shu darsning kodida atayin chiqarib ko'rdim:

```
# #include <GLFW/glfw3.h> olib tashlansa (menyu yo'q) — compiler xatosi:
main.cpp:12:10: error: 'glfwInit' was not declared in this scope

# CMake'da target_link_libraries olib tashlansa (oshxona yo'q) — linker xatosi:
ld.exe: main.cpp.obj: undefined reference to `glfwInit'
```

Bu farq kurs davomida ko'p asqotadi: xato qaysi bosqichda chiqqanini bilsangiz, sababning yarmini
topgan bo'lasiz. Mac'dagi clang xuddi shu xatolarni boshqacha yozadi: `use of undeclared identifier`
(compiler) va `Undefined symbols for architecture arm64` (linker).

### Statik va dinamik ulash

Linker kutubxonani ikki usulda ulay oladi:

- **Statik** (`.a`): kutubxona kodi dastur ichiga nusxalanadi. Fayl kattaroq, lekin mustaqil.
- **Dinamik** (Windows'da `.dll`, Mac'da `.dylib`): dasturga faqat "menga `glfw3.dll` kerak" deb yoziladi,
  kodni esa operatsion tizim ishga tushirish paytida yuklaydi.

Bu darsning kodi bilan Windows'da o'lchadim: dinamik GLFW bilan `fluid_sim.exe` 75 KB, statik GLFW bilan
415 KB. Dinamik holatda Windows `glfw3.dll`ni exe yonidan, tizim papkalaridan va `PATH`dan qidiradi.
Shuning uchun `C:\msys64\mingw64\bin` `PATH`da turishi shart (sizda bor).

## 2. CMake va Ninja: nega `g++ main.cpp` yetmaydi

Bu darsning `main.cpp`sini yig'ish uchun Windows'da aslida shu buyruqlar bajariladi (qisqartirib
ko'rsatdim):

```
c++ -DGLFW_DLL -O3 -DNDEBUG -std=gnu++17 -Wall -Wextra -MD ... -c src/main.cpp -o main.cpp.obj
c++ -O3 -DNDEBUG main.cpp.obj -o fluid_sim.exe C:/msys64/mingw64/lib/libglfw3.dll.a -lkernel32 -luser32 -lgdi32 ...
```

Mac'da compiler boshqa (`clang++`), yo'llar boshqa, tizim kutubxonalari boshqa. Bundan tashqari, loyiha
o'sib o'nlab fayl bo'lganda, faqat o'zgargan fayllarni qayta kompilyatsiya qilish kerak bo'ladi. Buni
qo'lda qilish mumkin emas.

Shu muammoni uch qatlam hal qiladi:

```
CMakeLists.txt ──(cmake: configure)──► build/build.ninja ──(ninja: build)──► g++ / clang++ ──► dastur
```

- **`CMakeLists.txt`**: loyihaning platformaga bog'liq bo'lmagan tavsifi: "`fluid_sim` degan dastur bor,
  fayllari mana bular, GLFW kerak".
- **CMake**: compiler yoki yig'uvchi emas, u *build system generator*. Tavsifni o'qiydi, kompyuterdagi
  compiler va kutubxonalarni topadi va aniq buyruqlar ro'yxatini (`build/build.ninja`) yozadi.
- **Ninja**: shu ro'yxatni bajaradi va faqat o'zgargan narsani qayta yig'adi.

Oshxonaga qaytsak: `CMakeLists.txt` umumiy retsept, `build.ninja` shu oshxonadagi aniq qadamlar ro'yxati,
Ninja esa tayyor taomni qayta pishirmaydigan oshpaz. Hech narsa o'zgarmagan bo'lsa, Ninja
`ninja: no work to do.` deydi.

**`build/` papkasi.** CMake yaratgan hamma narsa alohida `build/` papkasiga tushadi (*out-of-source build*).
Manba kod toza qoladi, `build/`ni esa istalgan payt o'chirib, qaytadan yaratish mumkin. Unda shu kompyuterning
absolyut yo'llari bor, shuning uchun u Git'ga kirmaydi: Windows'dagi `build/` Mac'da baribir ishlamaydi.

## 3. GLFW, oyna va hodisalar

**OpenGL oyna ochishni bilmaydi.** U faqat chizadi. Oyna operatsion tizimniki, va har bir tizimning usuli
har xil: Windows'da Win32 API (`CreateWindowExW`), Mac'da Cocoa (Objective-C dagi `NSWindow`). Klaviatura
va sichqonchani o'qish ham shunday. Bizga bir xil kod kerak, shuning uchun o'rtada **GLFW** turadi: u
ikkala tizimning farqini yashiradi va bizga bitta C API beradi. Keyingi darsda GLFW OpenGL kontekstini
ham yaratib beradi.

**Hodisalar navbati (event queue).** Foydalanuvchi oynani sudrasa, tugma bossa yoki X ni bossa, tizim
dasturga xabar yuboradi. Xabar o'z-o'zidan bajarilmaydi: u navbatga tushadi, xuddi pochta qutisiga tushgan
xatdek. Dastur qutini o'zi muntazam bo'shatib turishi kerak. Bo'shatmasa, tizim dastur osilib qoldi deb
hisoblaydi.

Sikl ichiga nima yozilishiga qarab farqni bu darsning kodida Windows'da (i5-12400F) o'lchadim:

| Sikl ichida | 2 soniyada sarflangan CPU vaqti | Oyna holati | X tugmasi |
|---|---|---|---|
| `glfwWaitEvents()` | 0 ms | javob beradi | ishlaydi |
| `glfwPollEvents()` | ~2000 ms (bitta yadro 100%) | javob beradi | ishlaydi |
| hech narsa | ~2000 ms | Windows uni osilgan deb belgiladi | ishlamaydi |

`glfwWaitEvents` xat kelguncha uxlaydi, `glfwPollEvents` esa qutini tekshiradi va darhol qaytadi. Hozir
chizadigan narsamiz yo'q, shuning uchun uxlaymiz. Keyingi darsda har kadrda chizamiz va `glfwPollEvents`ga
o'tamiz.

**Oyna va OpenGL konteksti bir narsa emas.** Oyna bu tizimdagi to'rtburchak joy. OpenGL konteksti esa
alohida obyekt: chizish holati va GPU bilan aloqa. Bu darsda oynani ataylab kontekstsiz ochamiz
(`GLFW_NO_API`), keyingi darsda kontekst qo'shamiz.

## 4. Git: ikki kompyuter orasidagi ko'prik

Git loyihaning tarixini saqlaydi. Har bir commit bu butun loyihaning bir lahzadagi surati. GitHub esa shu
tarixning uchinchi nusxasi: Windows'dan unga yuklaysiz, Mac'da undan tortib olasiz.

```
fayllar ──(git add)──► staging ──(git commit)──► mahalliy tarix ──(git push)──► GitHub ──(git pull)──► Mac
```

Qoida: Git'ga **odam yozgan narsa** kiradi. Qayta yaratish mumkin bo'lgan narsa (`build/`), kompyuterga
xos fayllar (`.DS_Store`) va shaxsiy muharrir sozlamalari kirmaydi.

## 5. Loyihani quramiz

### 5.1. Asboblar

**Windows.** MSYS2'dagi hamma narsa sizda o'rnatilgan (tekshirdim: g++ 16.2, CMake 4.4, Ninja 1.13,
GLFW 3.5.1). Boshqa kompyuterda kerak bo'lsa, MSYS2 terminalida:

```
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-glfw
```

**macOS.** Xcode Command Line Tools (`clang++` va `git`) va [Homebrew](https://brew.sh) kerak:

```
xcode-select --install
brew install cmake ninja glfw
```

### 5.2. Papka va Git

Loyiha papkasida `src` papkasini yarating va repo'ni boshlang:

```
git init -b main
```

`-b main` asosiy branch nomini belgilaydi.

### 5.3. `.gitignore`

Fayl nomi nuqtadan boshlanadi, loyihaning ildizida turadi:

```gitignore
build/
.vs/
.vscode/
.idea/
.DS_Store
```

`build/` yig'ish natijalari. `.vs/`, `.vscode/`, `.idea/` muharrirlarning shaxsiy sozlamalari. `.DS_Store`
Finder har bir papkada yaratadigan keraksiz fayl.

### 5.4. `CMakeLists.txt`

Bu fayl ham loyihaning ildizida turadi:

```cmake
cmake_minimum_required(VERSION 3.24)
project(fluid_sim LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(glfw3 3.3 QUIET)
if(NOT glfw3_FOUND)
    message(STATUS "GLFW topilmadi, GLFW 3.4 yuklab olinadi")
    set(GLFW_BUILD_DOCS OFF)
    include(FetchContent)
    FetchContent_Declare(glfw
        URL https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.zip
        URL_HASH SHA256=b5ec004b2712fd08e8861dc271428f048775200a2df719ccf575143ba749a3e9)
    FetchContent_MakeAvailable(glfw)
endif()

add_executable(fluid_sim
    src/main.cpp)

target_compile_options(fluid_sim PRIVATE -Wall -Wextra)
target_link_libraries(fluid_sim PRIVATE glfw)
```

Qismma-qism:

**`cmake_minimum_required(VERSION 3.24)`** ikki ish qiladi: eskiroq CMake'da yig'ishni darhol to'xtatadi va
CMake'ning xatti-harakatini shu versiyadagidek qiladi. Shu fayl yillar o'tib yangi CMake'da ham bir xil
ishlashi uchun kerak.

**`project(fluid_sim LANGUAGES C CXX)`** loyihani nomlaydi va compiler'larni qidiradi. `CXX` bu C++.
`C` kerak, chunki GLFW C tilida yozilgan va pastdagi zaxira yo'lda manbadan yig'iladi.

**`CMAKE_CXX_STANDARD 17`** compiler'ga C++17 ni buyuradi (yuqoridagi `-std=gnu++17` shundan keladi).
`REQUIRED ON` esa compiler 17 ni bilmasa, jimgina eskirog'iga tushib ketmasdan xato berishni aytadi.

**`CMAKE_BUILD_TYPE Release`** optimizatsiyani yoqadi: `-O3 -DNDEBUG`. Bu blokni olib tashlab ko'rdim,
buyruqda birorta `-O` qolmadi, ya'ni kod optimizatsiyasiz yig'iladi va hisob-kitobga to'la simulyatsiya
sezilarli sekin ishlaydi. `if(NOT ...)` esa kerak bo'lganda `-DCMAKE_BUILD_TYPE=Debug` bilan boshqasini
tanlash imkonini qoldiradi.

**`find_package(glfw3 3.3 QUIET)`** tizimda o'rnatilgan GLFW'ni (3.3 yoki yangiroq) qidiradi. Topsa,
`glfw3_FOUND` rost bo'ladi va `glfw` degan *target* paydo bo'ladi. `QUIET`: topilmasa ogohlantirish
chiqarma, buni o'zimiz hal qilamiz.

**`if(NOT glfw3_FOUND)` bloki** zaxira yo'l: GLFW o'rnatilmagan bo'lsa, CMake uni GitHub'dan yuklab,
loyiha bilan birga statik yig'adi. `URL_HASH` yuklangan fayl aynan biz kutgan fayl ekanini tekshiradi
(buzilgan yoki o'zgartirilgan bo'lsa, yig'ish to'xtaydi). `GLFW_BUILD_DOCS OFF` GLFW hujjatlarini
yig'masin deydi. Natijada `glfw` target'i xuddi shu nom bilan paydo bo'ladi, shuning uchun pastdagi
qatorlar ikkala yo'lda ham bir xil. Bu yo'lni ham Windows'da tekshirdim: 24 qadamda yig'ildi.

**`add_executable(fluid_sim src/main.cpp)`** dastur target'ini e'lon qiladi. Yangi `.cpp` qo'shilganda
uni shu ro'yxatga yozamiz, aks holda linker `undefined reference` beradi.

**`target_compile_options(... -Wall -Wextra)`** compiler ogohlantirishlarini yoqadi. Ishlatilmagan o'zgaruvchi
yoki shubhali o'zlashtirish kabi xatolarni arzon topish uchun.

**`target_link_libraries(fluid_sim PRIVATE glfw)`** dasturni GLFW bilan bog'laydi. Bu yerda `glfw` shunchaki
fayl nomi emas, **target**: unda include yo'li, kutubxona fayli va kerakli `#define`lar bor. Yuqoridagi
buyruqdagi `-DGLFW_DLL` va `libglfw3.dll.a`ni aynan shu bitta qator olib keldi. `PRIVATE`: bu bog'liqlik
faqat `fluid_sim`ning ichki ishi.

### 5.5. `src/main.cpp`

```cpp
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

int main() {
    glfwSetErrorCallback(on_glfw_error);

    if (!glfwInit()) {
        return 1;
    }
    std::printf("GLFW: %s\n", glfwGetVersionString());

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fluid Sim", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    while (!glfwWindowShouldClose(window)) {
        glfwWaitEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

Qismma-qism:

**`#define GLFW_INCLUDE_NONE`** `#include`dan **oldin** turishi kerak. Odatda `glfw3.h` tizimning eski OpenGL
header'ini ham qo'shib oladi. Bizga u kerak emas: keyingi darsda OpenGL funksiyalarini o'zimiz yuklaymiz va
ikki xil e'lon to'qnashmasligi kerak.

**`on_glfw_error`** bu *callback*: funksiyani GLFW'ga beramiz, xato bo'lganda GLFW uni o'zi chaqiradi.
`static` bu funksiya faqat shu fayl ichida ko'rinishini bildiradi.

**`glfwSetErrorCallback(...)`** `glfwInit`dan **oldin** chaqiriladi. Bu oldin chaqirish mumkin bo'lgan kam
sonli funksiyalardan biri, va shu tufayli `glfwInit`ning o'z xatolarini ham ko'ramiz. Busiz GLFW xatosi
jimgina o'tib ketadi va siz faqat "oyna ochilmadi" ni ko'rasiz, sababini emas.

**`glfwInit()`** GLFW'ni tizimning oyna xizmatiga ulaydi. Muvaffaqiyatsiz bo'lsa, `GLFW_FALSE` qaytaradi.

**`glfwGetVersionString()`** versiyani va GLFW qaysi tizim qatlamida ishlayotganini aytadi. Bir xil kod
Windows'da `Win32`, Mac'da `Cocoa` deb yozganini ko'rasiz.

**`glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API)`** keyingi yaratiladigan oynaga talab: "OpenGL konteksti kerak
emas". Hint'lar `glfwCreateWindow`dan oldin beriladi.

**`glfwCreateWindow(1280, 720, "Fluid Sim", nullptr, nullptr)`**: kenglik, balandlik, sarlavha. To'rtinchi
parametr monitor (`nullptr` oddiy oyna, monitor berilsa to'liq ekran). Beshinchisi resurslarini bo'lishadigan
boshqa oyna (bizda yo'q). Xato bo'lsa `nullptr` qaytaradi, shuning uchun tekshiramiz.

**Sikl.** X bosilganda GLFW oynani o'zi yopmaydi, faqat "yopilishi kerak" bayrog'ini ko'taradi. Bayroqni
`glfwWindowShouldClose` o'qiydi. Qaror dasturniki, masalan, chiqishdan oldin biror narsani saqlash mumkin.

**`glfwDestroyWindow` va `glfwTerminate`** oynani va GLFW resurslarini bo'shatadi. `return 0` tizimga "dastur
muvaffaqiyatli tugadi" deydi.

### 5.6. Yig'ish va ishga tushirish

Buyruqlar ikkala kompyuterda bir xil, loyihaning ildizida bajariladi:

```
cmake -S . -B build -G Ninja
cmake --build build
```

Birinchi buyruq *configure*: `-S .` manba shu yerda, `-B build` natija `build/`ga, `-G Ninja` qaysi yig'uvchi
uchun fayl yozish. `-G Ninja`siz CMake har bir tizimda boshqa narsani tanlagan bo'lardi.

Ikkinchi buyruq *build*. Keyinchalik, kodni o'zgartirganda, faqat shu ikkinchisi yetarli: `CMakeLists.txt`
o'zgargan bo'lsa, CMake o'zini o'zi qayta ishga tushiradi. Birinchisi faqat `build/` hali yo'q bo'lganda kerak.

Ishga tushirish:

```
.\build\fluid_sim.exe      # Windows (PowerShell)
./build/fluid_sim          # macOS
```

## 6. Ishlaganini qanday tekshirasiz

**Windows.** Bu natijani o'zim ko'rdim:

- "Fluid Sim" sarlavhali 1280×720 oyna ochiladi. Ichi oq, lekin rangning ahamiyati yo'q: uni hech kim
  chizmayapti.
- Terminalda: `GLFW: 3.5.1 Win32 WGL Null EGL OSMesa MinGW-w64 DLL`. Bu yerda `Win32` tizim qatlami, `DLL`
  GLFW dinamik ulanganini bildiradi.
- X bosilganda oyna yopiladi, terminal qaytadan buyruq kutadi, `echo $LASTEXITCODE` esa `0` beradi.
- Dastur kutib turganda Task Manager'da CPU taxminan 0%.

Dastur qaysi DLL'larga bog'liqligini ko'rish uchun:

```
objdump -p build\fluid_sim.exe | Select-String "DLL Name"
```

Ro'yxatda `glfw3.dll` bo'ladi. Zaxira yo'l (statik GLFW) bilan yig'ilganda u yo'q edi.

**macOS.** Mac'da kodni o'zim ishga tushira olmadim, shuning uchun bu qism kutilayotgan natija:

- "Fluid Sim" sarlavhali oyna ochiladi, ichining rangi muhim emas.
- Terminaldagi qatorda `Cocoa` so'zi bo'ladi. Versiya Homebrew'dagi GLFW'ga bog'liq.
- Qizil tugma oynani yopadi va dastur tugaydi.

`otool -L build/fluid_sim` ro'yxatida `libglfw` bo'ladi.

## 7. GitHub va Mac

**1. GitHub'da repo yarating.** [github.com/new](https://github.com/new) sahifasida nomi
`opengl_fluid_sim_scratch` bo'lsin va **bo'sh** qolsin (README, .gitignore va litsenziya qo'shmang,
aks holda birinchi push rad etiladi).

**2. Windows'da yuklang:**

```
git add .
git status
git commit -m "Lesson 00: project setup and empty window"
git remote add origin https://github.com/sardor-c/opengl_fluid_sim_scratch.git
git push -u origin main
```

`git status`da `build/` bo'lmasligi kerak: `.gitignore` ishlayotganining isboti shu.

`git add` paytida `LF will be replaced by CRLF` degan ogohlantirish chiqishi mumkin, zarari yo'q. Windows
qator oxirini ikki belgi (CRLF) bilan, Mac bir belgi (LF) bilan yozadi. Git bu farqni o'zi tekislaydi. Shader
fayllarini o'qiganda bu bizga bir marta kerak bo'ladi (2-dars).

**3. Mac'da oling va yig'ing:**

```
git clone https://github.com/sardor-c/opengl_fluid_sim_scratch.git
cd opengl_fluid_sim_scratch
cmake -S . -B build -G Ninja
cmake --build build
./build/fluid_sim
```

Toza klondan yig'ilishi muhim sinov: Git'ga biror fayl kirmay qolgan bo'lsa, shu yerda bilinadi.

**Har kungi tartib:** ishni `git pull` bilan boshlang, `git add .`, `git commit`, `git push` bilan tugating.

## 8. Ixtiyoriy tajribalar

1. `glfwWaitEvents();` qatorini vaqtincha o'chirib, qayta yig'ing. Taxminan 5 soniyadan keyin Windows oynani
   "javob bermayapti" deb belgilaydi va dastur X ga javob qaytarmaydi (Windows dasturni majburan yopishni
   taklif qilishi mumkin). Dasturni terminalda `Ctrl+C` bilan to'xtating.
2. `cmake --build build --clean-first --verbose` buyrug'i bilan CMake yozgan haqiqiy compiler buyruqlarini
   ko'ring va ularda `-O3`, `-std=gnu++17`, `-DGLFW_DLL` ni toping.

## Keyingi dars

01-darsda oynaga OpenGL 4.1 core kontekstini qo'shamiz: birinchi marta GPU bilan gaplashamiz, OpenGL
funksiyalarini yuklaymiz va oynani rangga bo'yaymiz.

Shu dars tugagach yozing. Xohlasangiz, kodingizni ko'rib chiqaman.
