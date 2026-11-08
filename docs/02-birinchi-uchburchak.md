# 02. Birinchi uchburchak: shader, VBO va VAO

## Bu qadamda nima qo'shamiz va nega aynan hozir

01-darsda GPU'ga faqat "hammasini bir rangga bo'ya" dedik. Endi unga o'z dasturimizni (shader) va o'z
ma'lumotimizni (uchta nuqta) beramiz, u esa rangli uchburchak chizadi.

Keyingi hamma chizish shu mexanizmga tayanadi. 03-darsda bitta zarracha ikkita uchburchak bo'ladi, minglab zarracha
esa ularning nusxalari. III qismda simulyatsiyaning o'zi ham shader'lar ichiga ko'chadi.

Dars oxirida loyihaga shular qo'shiladi:

```
opengl_fluid_sim_scratch/
├── CMakeLists.txt          (o'zgaradi)
├── shaders/                (yangi)
│   ├── triangle.vert
│   └── triangle.frag
└── src/
    ├── gl.hpp              (o'zgaradi)
    ├── shader.hpp          (yangi)
    ├── shader.cpp          (yangi)
    └── main.cpp            (o'zgaradi)
```

## 1. GPU qanday chizadi: pipeline

GPU alohida protsessor: o'z xotirasi va o'z dasturlari bor, minglab kichik yadrosi bir xil ishni parallel bajaradi.
Unga "mana bu pikselni shunday bo'ya" deb bittalab aytmaymiz. Ikki narsa beramiz: ma'lumot va dastur. U ma'lumotni
zavod konveyeridek bosqichlardan o'tkazadi:

```
VBO: vertexlar
     │
     ▼
Vertex shader        ← biz yozamiz, har bir vertex uchun bir marta ishlaydi
     │
     ▼
Uchburchak yig'ish va clipping
     │
     ▼
Rasterizer           ← uchburchak qaysi piksellarni qoplashini topadi
     │
     ▼
Fragment shader      ← biz yozamiz, har bir qoplangan piksel uchun bir marta ishlaydi
     │
     ▼
Framebuffer
```

**Vertex** bu uchburchakning uchi va unga tegishli ma'lumotlar (o'rni, rangi). Bizda 3 ta vertex, demak vertex shader
3 marta ishlaydi. **Fragment** bu "piksel bo'lishga nomzod". Bizning uchburchak yuz minglab piksel qoplaydi, fragment
shader ham shuncha marta ishlaydi.

Nega aynan uchburchak? Uch nuqta doim bitta tekislikda yotadi va uchburchak doim qavariq, shuning uchun qoplash hisobi
sodda va tez. Har qanday sirtni uchburchaklarga bo'lish mumkin.

## 2. Koordinatalar: NDC

GPU bizning birliklarimizni bilmaydi. Vertex shader natijani `gl_Position`ga yozadi, GPU esa uni shu kvadratga
joylaydi (*Normalized Device Coordinates*):

```
            y = +1
              ▲
              │
x = -1 ───────┼───────► x = +1
              │
            y = -1
```

- Markaz (0, 0). `y` yuqoriga o'sadi, ekran piksellarida esa odatda pastga.
- Kvadratdan tashqaridagi qism kesib tashlanadi (*clipping*).
- Oyna qanday shaklda bo'lmasin, kvadrat butun oynani qoplaydi. Shuning uchun 1280×720 oynada uchburchak eniga cho'zilib
  ko'rinadi. 03-darsda buni proyeksiya bilan to'g'rilaymiz.
- `gl_Position` `vec4` turida. To'rtinchi komponent `w` perspektiva uchun (15-dars), 2D da doim 1.0.

## 3. GLSL va shader'lar orasidagi aloqa

GLSL shader tili, u C'ga o'xshaydi. Fayl `#version 410 core` bilan boshlanadi. Qiymat bosqichdan bosqichga shunday o'tadi:

- **`in`** bosqichga kiradigan qiymat. Vertex shader'ning `in`lari VBO'dan keladi.
- **`out`** bosqichdan chiqadigan qiymat. Vertex shader'ning `out`i rasterizer orqali fragment shader'ning xuddi shu
  nomli `in`iga boradi.

**Interpolation.** Vertex shader 3 marta ishlaydi, fragment shader esa yuz minglab marta. Oradagi piksellar uchun
qiymat qayerdan keladi? Rasterizer uchta vertex qiymatining og'irlikli o'rtachasini hisoblaydi: piksel qaysi uchga yaqin
bo'lsa, o'shaning hissasi katta. Rang silliq o'tishining sababi shu. Uchburchakning og'irlik markazida uchala hissa teng:
uchta rangning o'rtachasi hisobda (127, 145, 127), ekrandan o'qilgan piksel ham aynan (127, 145, 127) chiqdi.

## 4. Shader'ni kompilyatsiya va link qilish

GLSL matni dastur ishlab turganda drayver ichidagi compiler bilan kompilyatsiya qilinadi. GPU'lar har xil, shuning uchun
tayyor mashina kodini tarqatib bo'lmaydi. Sxema 00-darsdagi bilan bir xil:

```
triangle.vert ──(glCompileShader)──► vertex shader obyekti ───┐
                                                              ├──(glLinkProgram)──► program
triangle.frag ──(glCompileShader)──► fragment shader obyekti ─┘
```

Shader obyekti object file'ga, program esa `.exe`ga o'xshaydi.

**Xatoni o'zimiz so'rashimiz kerak.** `glCompileShader` hech narsa qaytarmaydi, `gl_check` ham bunday xatoni ko'rmaydi.
Natijani `glGetShaderiv` bilan, matnni `glGetShaderInfoLog` bilan olamiz. Buni yozmasak, xato shader ekranda shunchaki
bo'shliq bo'lib ko'rinadi. Bu darsning kodida atayin chiqargan xatolar:

```
# triangle.frag'da nuqta-vergul qoldirilsa:
triangle.frag kompilyatsiya xatosi:
0(9) : error C0000: syntax error, unexpected '}', expecting ',' or ';' at token "}"

# triangle.vert'da main bo'lmasa (kompilyatsiyadan o'tadi, link'da yiqiladi):
Link xatosi (triangle.vert, triangle.frag):
Vertex info
-----------
(0) : error C3001: no program defined
```

`0(9)` da `0` manba raqami, `9` qator. Mac'da format boshqacha: `ERROR: 0:9: ...`.

**Nega alohida fayllarda.** Shader matnini C++ ichida yozish ham mumkin edi. Fayl qulayroq: shader o'zgarganda C++ qayta
yig'ilmaydi, dasturni qayta ishga tushirish yetarli. Fayl yo'lini CMake compile definition orqali beradi.

## 5. VBO: GPU xotirasidagi baytlar

**VBO** (*Vertex Buffer Object*) GPU xotirasidagi oddiy baytlar bloki. Unda "o'rin" yoki "rang" degan tushuncha yo'q,
faqat baytlar.

OpenGL obyektlar bilan **bind** uslubida ishlaydi: avval obyektni bog'lash nuqtasiga qo'yamiz, keyingi buyruq o'sha nuqtaga
ta'sir qiladi.

```cpp
glBindBuffer(GL_ARRAY_BUFFER, vbo);                         // hozir shu buffer bilan ishlayman
glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);  // hozirgi buffer'ga shu baytlarni yoz
```

`glBufferData` ma'lumotni nusxalaydi, shuning uchun CPU'dagi massiv keyin kerak emas. `GL_STATIC_DRAW` drayverga maslahat:
"bir marta yozaman, ko'p marta o'qiyman". 04-darsda zarrachalar har kadrda yangilanadi va `GL_STREAM_DRAW`ga o'tamiz.

## 6. VAO: baytlarni qanday o'qish kerak

VBO sarlavhasiz jadvalga o'xshaydi. **VAO** (*Vertex Array Object*) o'sha jadvalning sarlavhalari: qaysi attribute qaysi
buffer'dan, qayerdan boshlab, qanday turda o'qiladi. Bizda bitta vertex 5 ta `float`, ya'ni 20 bayt:

```
bayt:   0    4    8    12   16   20   24
        ┌────┬────┬────┬────┬────┬────┬────┐
        │ x  │ y  │ r  │ g  │ b  │ x  │ y  │ ...
        └────┴────┴────┴────┴────┴────┴────┘
        │◄──── bitta Vertex ────►│
```

Tavsifni `glVertexAttribPointer` beradi:

| Parametr | position | color | Ma'nosi |
|---|---|---|---|
| `index` | 0 | 1 | shader'dagi `layout(location = N)` |
| `size` | 2 | 3 | nechta komponent |
| `type` | `GL_FLOAT` | `GL_FLOAT` | komponent turi |
| `normalized` | `GL_FALSE` | `GL_FALSE` | butun sonlarni 0..1 ga o'tkazish (float uchun kerak emas) |
| `stride` | 20 | 20 | bir vertex boshidan keyingisining boshigacha necha bayt |
| `pointer` | 0 | 8 | vertex boshidan necha bayt keyin turadi |

VAO'siz bu sozlamalarni har bir chizishdan oldin qaytarishga to'g'ri kelardi. VAO ularni bir marta eslab qoladi,
chizishda `glBindVertexArray(vao)` yetarli. Core profilda u majburiy.

Nozik joy: VAO buffer'ni `glVertexAttribPointer` chaqirilgan paytda eslab qoladi, ya'ni shu paytda `GL_ARRAY_BUFFER`ga
bog'langan buffer attribute'ga yozib qo'yiladi. Shuning uchun tartib: VAO, VBO, keyin `glVertexAttribPointer`.

Chizish buyrug'i `glDrawArrays(GL_TRIANGLES, 0, 3)`: "0-vertexdan boshlab 3 ta vertexni uchburchak qilib chiz".

## 7. Kodni yozamiz

### 7.1. `CMakeLists.txt`

`add_executable` blokini va uning ostidagi include qatorini shunday qiling:

```cmake
add_executable(fluid_sim
    src/main.cpp
    src/gl.cpp
    src/shader.cpp
)

target_include_directories(fluid_sim PRIVATE external/khronos)
target_compile_definitions(fluid_sim PRIVATE SHADER_DIR="${CMAKE_SOURCE_DIR}/shaders")
```

`target_compile_definitions` kodga `SHADER_DIR` degan matnli makro beradi, xuddi `#define SHADER_DIR "C:/.../shaders"`
yozilgandek. Yo'l har kompyuterda boshqa, shuning uchun uni kodga qo'lda yozmaymiz: CMake shu kompyuterdagi yo'lni qo'yadi.

### 7.2. `src/gl.hpp`

`GL_FUNCTIONS` ro'yxatini to'liq shu bilan almashtiring, faylning qolgani o'zgarmaydi:

```cpp
#define GL_FUNCTIONS(X)                                                \
    X(PFNGLGETERRORPROC, glGetError)                                   \
    X(PFNGLGETSTRINGPROC, glGetString)                                 \
    X(PFNGLVIEWPORTPROC, glViewport)                                   \
    X(PFNGLCLEARCOLORPROC, glClearColor)                               \
    X(PFNGLCLEARPROC, glClear)                                         \
    X(PFNGLCREATESHADERPROC, glCreateShader)                           \
    X(PFNGLSHADERSOURCEPROC, glShaderSource)                           \
    X(PFNGLCOMPILESHADERPROC, glCompileShader)                         \
    X(PFNGLGETSHADERIVPROC, glGetShaderiv)                             \
    X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog)                   \
    X(PFNGLDELETESHADERPROC, glDeleteShader)                           \
    X(PFNGLCREATEPROGRAMPROC, glCreateProgram)                         \
    X(PFNGLATTACHSHADERPROC, glAttachShader)                           \
    X(PFNGLLINKPROGRAMPROC, glLinkProgram)                             \
    X(PFNGLGETPROGRAMIVPROC, glGetProgramiv)                           \
    X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog)                 \
    X(PFNGLDELETEPROGRAMPROC, glDeleteProgram)                         \
    X(PFNGLUSEPROGRAMPROC, glUseProgram)                               \
    X(PFNGLGENBUFFERSPROC, glGenBuffers)                               \
    X(PFNGLDELETEBUFFERSPROC, glDeleteBuffers)                         \
    X(PFNGLBINDBUFFERPROC, glBindBuffer)                               \
    X(PFNGLBUFFERDATAPROC, glBufferData)                               \
    X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays)                     \
    X(PFNGLDELETEVERTEXARRAYSPROC, glDeleteVertexArrays)               \
    X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray)                     \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray)     \
    X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer)             \
    X(PFNGLDRAWARRAYSPROC, glDrawArrays)
```

Hammasi OpenGL 4.1 da bor. Oxirgi qatorda `\` yo'q.

### 7.3. `shaders/triangle.vert`

```glsl
#version 410 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

out vec3 v_color;

void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);
    v_color = in_color;
}
```

`layout(location = N)` attribute raqamini belgilaydi, C++ dagi `glVertexAttribPointer`ning birinchi parametri shu raqamga
mos keladi. `vec4(in_position, 0.0, 1.0)` 2 ta sondan 4 komponentli vektor yasaydi: `z = 0`, `w = 1`. Nomlardagi `in_`
buffer'dan kelganini, `v_` bosqichlar orasida yurganini bildiradi (bu faqat bizning kelishuv).

### 7.4. `shaders/triangle.frag`

```glsl
#version 410 core

in vec3 v_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(v_color, 1.0);
}
```

`v_color` vertex shader'dagi `out` bilan bir xil nom va turda bo'lishi kerak. Natija `out_color` framebuffer'ga yoziladi,
to'rtinchi komponent shaffoflik.

### 7.5. `src/shader.hpp` (yangi fayl)

```cpp
#pragma once

#include "gl.hpp"

GLuint create_program(const char* vertex_file, const char* fragment_file);
```

### 7.6. `src/shader.cpp` (yangi fayl)

```cpp
#include "shader.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

static GLuint compile_shader(GLenum stage, const char* file) {
    const std::string path = std::string(SHADER_DIR) + "/" + file;
    std::ifstream stream(path);
    if (!stream) {
        std::fprintf(stderr, "Shader fayli ochilmadi: %s\n", path.c_str());
        return 0;
    }
    std::stringstream buffer;
    buffer << stream.rdbuf();
    const std::string source = buffer.str();
    const char* text = source.c_str();

    GLuint shader = glCreateShader(stage);
    glShaderSource(shader, 1, &text, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "%s kompilyatsiya xatosi:\n%s\n", file, log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint create_program(const char* vertex_file, const char* fragment_file) {
    GLuint vertex = compile_shader(GL_VERTEX_SHADER, vertex_file);
    GLuint fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_file);
    if (!vertex || !fragment) {
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Link xatosi (%s, %s):\n%s\n", vertex_file, fragment_file, log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}
```

**Faylni o'qish.** `rdbuf` butun faylni `stringstream`ga quyadi. `static`: bu funksiya faqat shu faylning ichki ishi.

**CRLF haqida tuzatish.** 00-darsda qator oxirlari shader o'qishda kerak bo'ladi degan edim, tekshirganda kerak emasligi
chiqdi. Shader'larni CRLF bilan sinadim, GLSL compiler ularni oddiy qator oxiri deb qabul qildi. Bu masala faqat qatorlarni
o'zimiz tahlil qila boshlaganda paydo bo'ladi.

**`glShaderSource(shader, 1, &text, nullptr)`**: bir nechta matn bo'lagini qabul qiladi, bizda bitta. `nullptr` "matn nol
belgi bilan tugaydi" degani.

**`glGetShaderiv`** nomidagi `iv`: natija butun son va ko'rsatkich orqali qaytadi. Ko'p OpenGL nomlari shunday tuzilgan.

**`create_program`** ikkala shader'ni ham kompilyatsiya qiladi, shuning uchun ikkalasidagi xatolar birdan ko'rinadi.
`glDeleteShader(0)` OpenGL'da jimgina e'tiborsiz qoldiriladi, shuning uchun ikkalasini tekshirmasdan o'chiramiz.

**Link'dan keyingi `glDeleteShader`.** Program shader kodini o'zida saqlab qoldi, oraliq obyektlar endi kerak emas, xuddi
`.exe` tayyor bo'lgach `.obj` kerak bo'lmagandek.

### 7.7. `src/main.cpp` (to'liq almashtiring)

```cpp
#include "gl.hpp"
#include "shader.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cstddef>
#include <cstdio>

struct Vertex {
    float x, y;
    float r, g, b;
};

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

    GLuint program = create_program("triangle.vert", "triangle.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const Vertex vertices[] = {
        {-0.6f, -0.5f, 1.0f, 0.25f, 0.25f},
        {0.6f, -0.5f, 0.25f, 1.0f, 0.25f},
        {0.0f, 0.6f, 0.25f, 0.45f, 1.0f},
    };

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, r)));
    gl_check("sozlash");

    double fps_time = glfwGetTime();
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        const double now = glfwGetTime();
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

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

01-darsdan farqlari:

**`struct Vertex`** VBO'dagi baytlarning C++ dagi tavsifi. Besh `float` oralig'ida bo'sh joy yo'q, shuning uchun
`sizeof(Vertex)` 20 ga teng va massiv xotirada 6-bo'limdagi rasmdek yotadi.

**`create_program`** kontekst va `gl_load`dan keyin chaqiriladi, chunki u OpenGL funksiyalarini ishlatadi.

**`glGenVertexArrays`, `glGenBuffers`** obyekt nomlarini (oddiy sonlarni) yaratadi. Birinchi parametr: nechta.

**`offsetof(Vertex, r)`** `r` maydoni struct boshidan necha baytda turishini beradi (8). Oxirgi parametrning turi tarixiy
sabab bilan ko'rsatkich: juda qadimda u CPU xotirasiga ko'rsatgan. Endi u buffer ichidagi bayt siljishi, shuning uchun
`reinterpret_cast` kerak.

**Kadr ichida** `glUseProgram` va `glBindVertexArray`ni har safar chaqiramiz. Hozir bitta obyekt bor, bir marta yetardi, lekin
keyin bir nechta program va obyekt bo'ladi, va chizish "qaysi holat o'rnatilgan bo'lsa, shuni ishlatadi".

**O'chirish** `glfwDestroyWindow`dan oldin turadi, chunki OpenGL chaqiruvlari uchun kontekst tirik bo'lishi kerak.

### 7.8. Yig'ish va ishga tushirish

```
cmake --build build
.\build\fluid_sim.exe      # Windows (PowerShell)
```

## 8. Ishlaganini qanday tekshirasiz

**Windows.** Bu natijani o'zim ko'rdim:

- To'q fonda rangli uchburchak: chap past burchak qizil, o'ng past yashil, tepa ko'k, orasi silliq o'tadi.
- Terminalda `OpenGL xatosi` qatorlari yo'q, sarlavhada 165 FPS.
- Oyna eniga cho'zilgan shaklni ko'rsatadi, bu 2-bo'limda aytilganidek kutilgan holat.

**macOS.** Buni o'zim ishga tushira olmadim, kutilayotgan natija: xuddi shu uchburchak, oynaning xuddi shu joyida. Oyna
to'liq to'lgani `glViewport`ga framebuffer o'lchami berilganining isboti.

**Nima buzilsa nima bo'ladi.** Quyidagilarning hammasini RTX 3060 da sinadim:

| O'zgarish | Natija | Xabar bormi |
|---|---|---|
| `glBindVertexArray` yo'q | hech narsa chizilmaydi | `GL_INVALID_OPERATION` |
| `stride` 0 | shakl ham, rang ham buziladi | yo'q |
| `glUseProgram` yo'q | qora uchburchak | yo'q |
| `glViewport` yarim o'lchamda | rasm chap pastki choragiga siqiladi (Retina xatosi shu ko'rinishda bo'ladi) | yo'q |
| `triangle.frag`da `v_colour` deb yozilsa | qora uchburchak, link o'tadi | yo'q |
| `triangle.frag`da `vec4 v_color` | link o'tadi | yo'q |
| `#version 430 core` | ishlaydi | yo'q |

Oxirgi uch qatorga e'tibor bering: NVIDIA drayveri kechirimli. GLSL talab qiladigan moslikni tekshirmasdan o'tkazib yuborishi
mumkin. Apple'ning compiler'i qattiqroq, bunday kod Mac'da odatda xato beradi (buni o'zim ko'ra olmadim). Shuning uchun
har doim `#version 410 core` yozamiz va har bir qadamni ikkala kompyuterda sinaymiz.

## 9. Git va Mac

Windows'da:

```
git add .
git commit -m "Lesson 02: first triangle with shaders"
git push
```

Mac'da:

```
git pull
cmake --build build
./build/fluid_sim
```

## 10. Ixtiyoriy tajribalar

1. `triangle.frag`da `out_color = vec4(1.0, 0.5, 0.0, 1.0);` deb yozing va **yig'masdan**, dasturni qayta ishga tushiring.
   Uchburchak to'q sariq bo'ladi: shader fayldan o'qilishining foydasi shu.
2. Mac'da `triangle.frag`dagi ikkala `v_color`ni vaqtincha `v_colour` qiling. Windows'da uchburchak qora chiqqan edi, Mac'da
   nima chiqqanini yozing.

## Keyingi dars

03-darsda uchburchakdan zarrachaga o'tamiz: dunyo koordinatalarini ekranga o'tkazadigan orthographic proyeksiya, bitta chizish
buyrug'i bilan minglab nusxa (instancing) va silliq chetli doira.

Shu dars tugagach yozing. Xohlasangiz, kodingizni ko'rib chiqaman.
