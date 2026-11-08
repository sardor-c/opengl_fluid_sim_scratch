# 01. OpenGL konteksti: GPU bilan birinchi suhbat

## Bu qadamda nima qo'shamiz va nega aynan hozir

00-darsdagi oynani hech kim chizmasdi. Endi unga OpenGL 4.1 core kontekstini ulaymiz, OpenGL funksiyalarini
yuklaymiz va oynani har kadrda rangga bo'yaymiz. Sarlavhada FPS va o'lchamlar chiqadi.

Uchburchak keyingi darsda. Undan oldin shu poydevor kerak: kontekstsiz birorta `gl...` funksiya ishlamaydi,
yuklanmagan funksiya esa dasturni yiqitadi.

Dars oxirida loyihaga shular qo'shiladi:

```
opengl_fluid_sim_scratch/
├── CMakeLists.txt          (o'zgaradi)
├── external/khronos/       (yangi: Khronos header'lari)
│   ├── GL/glcorearb.h
│   └── KHR/khrplatform.h
└── src/
    ├── gl.hpp              (yangi)
    ├── gl.cpp              (yangi)
    └── main.cpp            (o'zgaradi)
```

## 1. OpenGL aslida nima

OpenGL dastur ham, kutubxona ham emas. U **spetsifikatsiya**: Khronos guruhi yozgan hujjat, unda "`glClear`
chaqirilsa, mana bu bo'lishi kerak" deyilgan. Hujjatni bajaradigan kodni GPU drayveri beradi: Windows'da NVIDIA
drayveri, Mac'da Apple'ning Metal ustidagi qatlami. Bir xil `glClear` ikki kompyuterda butunlay boshqa kodni
ishga tushiradi, natija esa bir xil bo'ladi.

OpenGL **holat mashinasi** (state machine). Ko'p chaqiruvlar hech narsa chizmaydi, faqat sozlamani o'zgartiradi,
keyingi buyruqlar esa shu sozlamalardan foydalanadi. Bu darsda ham shunday: `glClearColor` faqat "tozalash rangi
shu" deb eslab qoladi, ishni `glClear` bajaradi.

## 2. Kontekst

Holat **kontekst**da saqlanadi. Kontekst bu bitta obyekt: OpenGL'ning hozirgi sozlamalari, yaratilgan resurslar
(buffer, tekstura, shader) va shu oynaga chizish imkoniyati.

Uni ustaxona deb tasavvur qiling: asboblar o'z joyida, ishlar stolda. `gl...` chaqiruvlarida "qaysi ustaxonada"
degan parametr yo'q, shuning uchun avval ichkariga kirib olish kerak. `glfwMakeContextCurrent(window)` aynan shuni
qiladi: "shu oqim (thread) endi shu kontekstda ishlaydi". Keyingi har bir `gl...` chaqiruv shu kontekstga boradi.

## 3. Versiya va profil: nega 4.1 core

Oynani yaratishdan oldin GLFW'ga qanday kontekst kerakligini hint bilan aytamiz:

| Hint | Qiymat | Ma'nosi |
|---|---|---|
| `GLFW_CONTEXT_VERSION_MAJOR`, `MINOR` | 4, 1 | OpenGL 4.1 kerak |
| `GLFW_OPENGL_PROFILE` | `GLFW_OPENGL_CORE_PROFILE` | eski API'siz |
| `GLFW_OPENGL_FORWARD_COMPAT` | `GLFW_TRUE` | "eskirgan" deb belgilangan hamma narsa ham olib tashlansin |

**Nega 4.1.** Apple OpenGL'ni 4.1 da to'xtatgan va keyin deprecated deb e'lon qilgan (hali ishlaydi). Bir xil kod
ikkala kompyuterda ishlashi uchun shift shu.

**Core profil.** OpenGL 1990-yillardan beri o'sib kelgan. Eski uslub (`glBegin`, `glVertex3f`) 3.2 versiyada
*compatibility* profilga ajratilgan. *Core* profilda u yo'q, hammasini buffer va shader bilan qilamiz. Mac'da
3.2 dan yuqori versiyalar faqat core profilda bor.

**Forward-compat.** GLFW 3.3 Mac'da bu hintsiz kontekst bermaydi. Uning manbasida shunday xato yozilgan:

```
NSGL: The targeted version of macOS only supports forward-compatible core profile contexts for OpenGL 3.2 and above
```

GLFW 3.4 da bu tekshiruv yo'q, lekin `CMakeLists.txt`da minimal versiya 3.3, shuning uchun bu bir qator qoladi.
Windows'da hintni olib tashlab ko'rdim: hech narsa o'zgarmadi.

**Drayver nima berdi.** RTX 3060 da `GL_VERSION` aynan `4.1.0 NVIDIA 610.47` qaytardi. Ammo drayver yangi
funksiyalarni yashirmaydi: yuklash ro'yxatiga `glDispatchCompute` (OpenGL 4.3) qo'shib ko'rdim va u Windows'da
topildi. Mac'da bunday funksiya yo'q. Demak, 4.1 chegarasini Windows bizga eslatmaydi, uni o'zimiz ushlab turamiz.

## 4. Nega funksiyalarni "yuklash" kerak

00-darsda GLFW funksiyalarini linker uladi, chunki kutubxona fayli bor edi. OpenGL'da bunday qilib bo'lmaydi:
haqiqiy kod drayverda, drayver esa har bir kompyuterda boshqa. Windows'ning `opengl32.dll` fayli faqat juda eski
OpenGL 1.1 ni eksport qiladi. O'zingizning kompyuteringizda tekshirdim:

| Funksiya | `opengl32.dll`da |
|---|---|
| `glClear`, `glViewport`, `glGetString` | bor |
| `glGenBuffers`, `glGenVertexArrays`, `glUseProgram` | yo'q |

Yechim: dastur ishlab turganda drayverdan so'raymiz: "`glGenBuffers`ning manzili qayerda?". Javob
**funksiya ko'rsatkichi** (function pointer): funksiyaning xotiradagi manzilini saqlaydigan o'zgaruvchi. Uni oddiy
funksiyadek chaqirish mumkin.

Manzilni faqat kontekst bor paytda so'rash mumkin, shuning uchun tartib qat'iy:

```
oyna + kontekst yaratish ──► glfwMakeContextCurrent ──► gl_load ──► gl... chaqiruvlari
```

Tartib buzilsa nima bo'lishini bu darsning kodida sinab ko'rdim:

```
# gl_load kontekstni joriy qilishdan OLDIN chaqirilsa:
GLFW xatosi 65538: Cannot query entry point without a current OpenGL or OpenGL ES context
OpenGL funksiyasi topilmadi: glGetError

# gl_load umuman chaqirilmasa: ko'rsatkich nullptr, birinchi gl... chaqiruvida dastur yiqiladi
Windows'da chiqish kodi 0xC0000005 (access violation), hech qanday xabarsiz
```

**`GL/glcorearb.h`** Khronos'ning rasmiy header'i. U konstantalarni (`GL_COLOR_BUFFER_BIT`) va funksiya ko'rsatkichi
turlarini beradi. Tur nomining qolipi: `PFN` + `GL` + katta harflar bilan funksiya nomi + `PROC`. Masalan, `glClear`
uchun `PFNGLCLEARPROC`. macOS SDK'da bu fayl yo'q, shuning uchun uni loyihaga qo'shamiz va ikkala kompyuter bir xil
faylni ishlatadi.

**Nega glad emas.** glad kabi generator bu ishni avtomatik qiladi. Biz qo'lda qilamiz: kod atigi bir necha qator,
mexanizm ko'rinib turadi, ro'yxatda faqat ishlatilgan funksiyalar bo'ladi va har bir yangi OpenGL funksiyasi loyihaga
ongli ravishda kiradi.

**X-macro.** Har bir funksiya uchun uch narsa kerak: e'lon, ta'rif va yuklash. Uchta alohida ro'yxat yuritsak,
birini unutish oson. Shuning uchun ro'yxat bitta, uni preprocessor uch xil ochadi. Ro'yxatdagi shu bitta qator:

```cpp
X(PFNGLCLEARPROC, glClear)
```

`X` nimaga teng qilib berilishiga qarab shu uchta kodga aylanadi:

```cpp
extern PFNGLCLEARPROC glClear;                                             // gl.hpp: "shunday o'zgaruvchi bor"
PFNGLCLEARPROC glClear = nullptr;                                          // gl.cpp: "mana o'sha o'zgaruvchi"
glClear = reinterpret_cast<PFNGLCLEARPROC>(glfwGetProcAddress("glClear")); // gl_load: manzilni so'rash
```

## 5. Ikki buffer va vsync

Kontekst bilan birga oynaga ikkita rasm beriladi. **Front buffer** ekranda ko'rinadi, **back buffer**ga esa chizamiz.
Chizish tugagach `glfwSwapBuffers` ularni almashtiradi. Rassomni tasavvur qiling: u ikkinchi kanvasda ishlaydi va
tomoshabinga faqat tayyor rasmni ko'rsatadi. Bir buffer bo'lganda foydalanuvchi yarim chizilgan kadrni ko'rardi.

**Vsync.** Monitor ekranni sekundiga N marta yuqoridan pastga yangilaydi. Almashtirish shu jarayonning o'rtasiga
to'g'ri kelsa, ekranning yuqorisida eski, pastida yangi kadr ko'rinadi (*tearing*). `glfwSwapInterval(1)` "navbatdagi
yangilanishni kutib almashtir" degani, u sikl tezligini monitorga bog'laydi.

Bu darsning kodida o'lchadim:

| Variant | FPS | Ekran |
|---|---|---|
| `glfwSwapInterval(1)` | 165 (monitoringiz chastotasi) | rang ko'rinadi |
| `glfwSwapInterval(0)` | ~11 800 | rang ko'rinadi |
| `glfwSwapBuffers` yo'q | ~110 000 | oq qoldi: hamma kadr back buffer'da qolib ketdi |

## 6. Oyna o'lchami va framebuffer o'lchami

Mac'ning Retina ekranida bitta oyna koordinatasi odatda 2 ta fizik piksel. Shuning uchun ikki xil o'lcham bor:

- `glfwGetWindowSize`: oyna koordinatalarida, masalan 1280×720. Sichqoncha shu birlikda ishlaydi (07-dars).
- `glfwGetFramebufferSize`: haqiqiy piksellarda, masalan 2560×1440.

OpenGL piksellar bilan ishlaydi, shuning uchun `glViewport`ga framebuffer o'lchami beriladi. `glViewport` chizish
framebuffer'ning qaysi to'rtburchagiga tushishini aytadi. Oyna o'lchami istalgan payt o'zgarishi mumkin, shuning
uchun uni har kadrda so'raymiz.

Bitta nozik joy: `glClear` viewport'ga qaramaydi va butun framebuffer'ni tozalaydi. Shuning uchun bu darsda
noto'g'ri `glViewport` ko'rinmaydi. Uning xatosi uchburchak chizganda chiqadi: Retina'da rasm ekranning chap pastki
choragiga siqilib qoladi.

## 7. Xatolarni ushlash: `glGetError`

Noto'g'ri OpenGL chaqiruvi dasturni to'xtatmaydi: buyruq e'tiborsiz qoldiriladi va ichki xato bayrog'i ko'tariladi.
Bilishning yagona yo'li `glGetError` bilan so'rash. U bayroqni qaytaradi va tozalaydi, bir nechta bayroq bo'lishi
mumkin, shuning uchun sikl bilan o'qiymiz.

Qulayroq *debug output* (KHR_debug) OpenGL 4.3 da paydo bo'lgan, Mac'da u yo'q. Shuning uchun kichik `gl_check`
yordamchisini yozamiz. `glClear(0xFFFFFFFF)` bilan sinadim, har kadrda shu chiqdi:

```
OpenGL xatosi (kadr): GL_INVALID_VALUE
```

## 8. Kodni yozamiz

### 8.1. Khronos header'lari

Loyihaning ildizida (Windows PowerShell'da):

```
curl.exe -L --create-dirs -o external/khronos/GL/glcorearb.h https://registry.khronos.org/OpenGL/api/GL/glcorearb.h
curl.exe -L --create-dirs -o external/khronos/KHR/khrplatform.h https://registry.khronos.org/EGL/api/KHR/khrplatform.h
```

`.exe` muhim: PowerShell'da oddiy `curl` boshqa buyruqqa ishora qiladi. Bu ikki fayl Git'ga kiradi, Mac ularni
`git pull` bilan oladi.

### 8.2. `CMakeLists.txt`

`add_executable` blokiga `src/gl.cpp`ni qo'shing va ostiga include yo'lini yozing:

```cmake
add_executable(fluid_sim
    src/main.cpp
    src/gl.cpp
)

target_include_directories(fluid_sim PRIVATE external/khronos)
```

`target_include_directories` compiler'ga `#include <GL/glcorearb.h>`ni shu papkadan qidirishni aytadi. `gl.cpp`
ro'yxatga kirmasa, linker `undefined reference to gl_load` beradi.

### 8.3. `src/gl.hpp` (yangi fayl)

```cpp
#pragma once

#if defined(_WIN32) && !defined(APIENTRY)
#define APIENTRY __stdcall
#endif
#include <GL/glcorearb.h>

#define GL_FUNCTIONS(X)                  \
    X(PFNGLGETERRORPROC, glGetError)     \
    X(PFNGLGETSTRINGPROC, glGetString)   \
    X(PFNGLVIEWPORTPROC, glViewport)     \
    X(PFNGLCLEARCOLORPROC, glClearColor) \
    X(PFNGLCLEARPROC, glClear)

#define X(type, name) extern type name;
GL_FUNCTIONS(X)
#undef X

bool gl_load();
void gl_check(const char* where);
```

**`APIENTRY` bloki.** Header `APIENTRY` makrosini talab qiladi: u funksiya chaqirish kelishuvini (parametrlar qayerga
qo'yilishini) bildiradi. Windows'da u aniqlanmagan bo'lsa, `glcorearb.h` butun `<windows.h>`ni qo'shib oladi. Bu
zararli: `<windows.h>` `near` va `far` degan bo'sh makrolar yaratadi. Bu blokni olib tashlab, `float near = 0.1f;`
yozib ko'rdim:

```
main.cpp:47:16: error: expected unqualified-id before '=' token
```

Kamera darsida aynan shunday nomlar kerak bo'ladi. Bu blok loyihadagi kam uchraydigan platformaga xos joy, u faqat
Windows'da ishlaydi va Mac'ga ta'sir qilmaydi.

**`GL_FUNCTIONS(X)`** yagona ro'yxat. `\` makroni keyingi qatorga davom ettiradi, shuning uchun oxirgi qatorda u yo'q.
Yangi OpenGL funksiyasi kerak bo'lganda shu ro'yxatga bitta qator qo'shamiz, boshqa hech narsa o'zgarmaydi.

**`#define X ... extern ...`** ro'yxatni e'lonlarga ochadi. `#undef X` nomni keyingi ochish uchun bo'shatadi.
Ko'rsatkichlar aynan funksiya nomi bilan atalgan, shuning uchun qolgan kod oddiy OpenGL kabi o'qiladi: `glClear(...)`.

### 8.4. `src/gl.cpp` (yangi fayl)

```cpp
#include "gl.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstdio>

#define X(type, name) type name = nullptr;
GL_FUNCTIONS(X)
#undef X

bool gl_load() {
#define X(type, name)                                                     \
    name = reinterpret_cast<type>(glfwGetProcAddress(#name));             \
    if (!name) {                                                          \
        std::fprintf(stderr, "OpenGL funksiyasi topilmadi: %s\n", #name); \
        return false;                                                     \
    }
    GL_FUNCTIONS(X)
#undef X
    return true;
}

void gl_check(const char* where) {
    while (GLenum error = glGetError()) {
        const char* name = "noma'lum";
        switch (error) {
            case GL_INVALID_ENUM: name = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: name = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: name = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: name = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY: name = "GL_OUT_OF_MEMORY"; break;
        }
        std::fprintf(stderr, "OpenGL xatosi (%s): %s\n", where, name);
    }
}
```

**Birinchi `#define X`** ko'rsatkichlarning o'zini yaratadi. `extern` faqat "shunday o'zgaruvchi bor" deydi, xotira esa
shu yerda ajratiladi va u butun dasturda bitta bo'ladi.

**`gl_load`.** `#name` makro parametrini matnga aylantiradi: `glClear` dan `"glClear"` hosil bo'ladi.
`glfwGetProcAddress` umumiy turdagi manzil qaytaradi, `reinterpret_cast` uni to'g'ri turga o'tkazadi. Windows'da GLFW
yangi funksiyalarni drayverdan, 1.1 funksiyalarini `opengl32.dll`dan oladi. Biz bu farqni ko'rmaymiz. Funksiya
topilmasa, nomini aytib to'xtaymiz: xato nomsiz crash bo'lib chiqqandan ko'ra shunisi yaxshi.

**`gl_check`.** `GL_NO_ERROR` qiymati 0, shuning uchun `while` xatolar tugaganda to'xtaydi. `where` xato qaysi joyda
ushlanganini ko'rsatish uchun.

### 8.5. `src/main.cpp` (to'liq almashtiring)

```cpp
#include "gl.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

static const char* gl_string(GLenum name) {
    return reinterpret_cast<const char*>(glGetString(name));
}

int main() {
    glfwSetErrorCallback(on_glfw_error);

    if (!glfwInit()) {
        return 1;
    }
    std::printf("GLFW:   %s\n", glfwGetVersionString());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Fluid Sim", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    if (!gl_load()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::printf("OpenGL: %s\n", gl_string(GL_VERSION));
    std::printf("GLSL:   %s\n", gl_string(GL_SHADING_LANGUAGE_VERSION));
    std::printf("GPU:    %s\n", gl_string(GL_RENDERER));

    glfwSwapInterval(1);

    double fps_time = glfwGetTime();
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        const double now = glfwGetTime();
        const float wave = 0.5f + 0.5f * static_cast<float>(std::sin(now));
        glClearColor(0.05f, 0.15f + 0.25f * wave, 0.35f + 0.45f * wave, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        if (now - fps_time >= 0.5) {
            int win_width = 0;
            int win_height = 0;
            glfwGetWindowSize(window, &win_width, &win_height);

            char title[128];
            std::snprintf(title, sizeof(title), "Fluid Sim | %.0f FPS | oyna %dx%d | framebuffer %dx%d",
                          fps_frames / (now - fps_time), win_width, win_height, fb_width, fb_height);
            glfwSetWindowTitle(window, title);

            fps_time = now;
            fps_frames = 0;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

00-darsdan farqlari:

**To'rtta hint** `GLFW_NO_API` o'rnini oldi (3-bo'lim). Ular `glfwCreateWindow`dan oldin turishi shart. So'ralgan kontekst
berilmasa, `glfwCreateWindow` `nullptr` qaytaradi va sababini `on_glfw_error` chiqaradi.

**`glfwMakeContextCurrent`, keyin `gl_load`**: 4-bo'limdagi tartib.

**`gl_string`.** `glGetString` `const GLubyte*` (ishorasiz baytlar) qaytaradi, `printf` esa `const char*` kutadi. Bu uch
qator qaysi kontekst olinganini aytadi va kelajakda muammo chiqsa birinchi qaraladigan joy.

**`glfwSwapInterval(1)`** joriy kontekstga qo'llanadi, shuning uchun `glfwMakeContextCurrent`dan keyin turadi.

**`glfwPollEvents`.** Endi har kadrda chizamiz, kutib o'tirmaymiz, shuning uchun `glfwWaitEvents` o'rniga shu.

**`wave`.** `sin` qiymati -1 dan 1 gacha, `0.5 + 0.5·sin` esa 0 dan 1 gacha. Rang vaqt bilan o'zgarib tursa, sikl
haqiqatan ishlayotganini va kadrlar ekranga yetib borayotganini ko'zimiz bilan ko'ramiz.

**`glClearColor`** qizil, yashil, ko'k va shaffoflikni 0 dan 1 gacha oladi. **`glClear(GL_COLOR_BUFFER_BIT)`** qaysi
bufferni tozalashni aytadi (keyinroq depth buffer ham qo'shiladi).

**FPS.** Kadrlarni sanaymiz va har 0.5 soniyada `kadrlar / vaqt`ni sarlavhaga yozamiz. Bu kurs davomida eng tez
ishlatiladigan o'lchov asbobimiz bo'ladi.

**`glfwDestroyWindow`** kontekstni ham yo'q qiladi, alohida o'chirish shart emas.

### 8.6. Yig'ish va ishga tushirish

`CMakeLists.txt` o'zgargani uchun CMake o'zini qayta ishga tushiradi, bitta buyruq yetarli:

```
cmake --build build
.\build\fluid_sim.exe      # Windows (PowerShell)
```

## 9. Ishlaganini qanday tekshirasiz

**Windows.** Bu natijani o'zim ko'rdim. Terminalda:

```
GLFW:   3.5.1 Win32 WGL Null EGL OSMesa MinGW-w64 DLL
OpenGL: 4.1.0 NVIDIA 610.47
GLSL:   4.10 NVIDIA via Cg compiler
GPU:    NVIDIA GeForce RTX 3060/PCIe/SSE2
```

- Oyna to'q ko'kdan och ko'kka asta o'zgarib turadi.
- Sarlavhada: `Fluid Sim | 165 FPS | oyna 1280x720 | framebuffer 1280x720`.
- Terminalda `OpenGL xatosi` qatorlari yo'q.
- X oynani yopadi, `echo $LASTEXITCODE` `0` beradi.

Oynani sichqoncha bilan sudrab turganda rang to'xtashi mumkin: Windows bu paytda hodisalar siklini ushlab turadi.
Bu xato emas.

**macOS.** Buni o'zim ishga tushira olmadim, quyidagilar kutilayotgan natija:

- `OpenGL:` qatori `4.1 Metal - ...` ko'rinishida, `GLSL:` qatori `4.10`, `GPU:` qatorida Mac'ingizning chipi.
- Rang xuddi shunday o'zgarib turadi.
- Sarlavhada `oyna` va `framebuffer` **har xil** bo'ladi (Retina), masalan `oyna 1280x720 | framebuffer 2560x1440`.
  6-bo'limdagi gapning isboti shu.

## 10. Git va Mac

Windows'da:

```
git add .
git commit -m "Lesson 01: OpenGL 4.1 context and function loading"
git push
```

Mac'da:

```
git pull
cmake --build build
./build/fluid_sim
```

## 11. Ixtiyoriy tajribalar

1. `glfwSwapInterval(1)` ni `glfwSwapInterval(0)` ga o'zgartirib, sarlavhadagi FPS'ga qarang.
2. `glClear(GL_COLOR_BUFFER_BIT)` ni vaqtincha `glClear(0xFFFFFFFF)` qilib, terminaldagi `gl_check` xabarini ko'ring.

## Keyingi dars

02-darsda birinchi uchburchakni chizamiz: GLSL 410 shader'lar, VAO va VBO, va GPU'ga ma'lumot yuborish.

Shu dars tugagach yozing. Xohlasangiz, kodingizni ko'rib chiqaman.
