# 03. Ko'p zarracha: dunyo koordinatalari, instancing va silliq doira

## Bu qadamda nima qo'shamiz va nega aynan hozir

02-darsda bitta uchburchakni NDC koordinatalarida chizdik. Suyuqlik uchun minglab doira kerak, ularning o'rni esa fizika
birliklarida bo'ladi. Oldimizda uchta muammo turibdi va dars shu uchtasini hal qiladi:

1. NDC fizika uchun noqulay va rasmni cho'zadi: **dunyo koordinatalari va proyeksiya** kerak.
2. Minglab obyektni bittalab chizish sekin: **instancing** kerak.
3. GPU doira chizishni bilmaydi: **quad, fragment shader va blending** kerak.

Dars oxirida ekranda 4536 ta doira turadi. 04-darsdan boshlab ular harakatlana boshlaydi, chizish kodi esa deyarli
o'zgarmaydi.

Loyihada shular o'zgaradi (`CMakeLists.txt`ga tegmaymiz):

```
opengl_fluid_sim_scratch/
├── shaders/
│   ├── particle2d.vert     (yangi)
│   ├── particle2d.frag     (yangi)
│   ├── triangle.vert       (o'chiriladi)
│   └── triangle.frag       (o'chiriladi)
└── src/
    ├── gl.hpp              (o'zgaradi)
    ├── shader.hpp          (o'zgaradi)
    ├── shader.cpp          (o'zgaradi)
    └── main.cpp            (o'zgaradi)
```

## 1. Dunyo koordinatalari

Fizikada o'lchamlar pikselda emas. Keyingi darslardagi sahnada idish 17.1 × 9.3 birlik, gravitatsiya −12, zarracha
diametri 0.08. Oyna kattalashsa fizika o'zgarmasligi kerak. Shuning uchun simulyatsiya **dunyo koordinatalarida** yashaydi,
ularni ekranga o'tkazish esa faqat chizishning ishi.

Dunyoning qaysi qismi ko'rinishini o'zimiz tanlaymiz: markazi (0, 0), balandligining yarmi `half_height = 5`, eni esa
oyna shakliga bog'liq.

```
         dunyo                                  NDC
(-8.89, 5) ─────────── (8.89, 5)      (-1, 1) ───────── (1, 1)
    │                     │               │                │
    │       (0, 0)        │     ──►       │     (0, 0)     │
    │                     │               │                │
(-8.89,-5) ─────────── (8.89,-5)      (-1,-1) ───────── (1,-1)
```

O'tkazish oddiy bo'lishdan iborat:

```
clip.x = world.x / half_width
clip.y = world.y / half_height
```

Buni **orthographic proyeksiya** deyishadi: uzoqlik o'lchamga ta'sir qilmaydi, perspektiva yo'q. To'liq shakli matritsa
bo'ladi (15-dars), lekin markazi nolda turgan 2D kamera uchun ikkita son yetadi. Shuning uchun shader'ga bitta `vec2`
beramiz: `(1 / half_width, 1 / half_height)`.

**`half_width` nega hisoblanadi.** NDC kvadrati butun oynaga cho'ziladi, 1280×720 oynada esa x o'qi uzunroq. Doira doira
bo'lib qolishi uchun 1 dunyo birligi ikkala o'qda bir xil piksel bo'lishi kerak. Buning uchun `half_width = half_height * aspect`,
bu yerda `aspect = fb_width / fb_height`. Tekshirib ko'ramiz:

```
aspect       = 1280 / 720        = 1.778
half_width   = 5 * 1.778         = 8.889
1 birlik (x) = (1280 / 2) / 8.889 = 72 piksel
1 birlik (y) = (720 / 2) / 5      = 72 piksel
```

`aspect`ni framebuffer o'lchamidan olamiz, chunki `glViewport`ga ham shu beriladi. Minimize qilingan oynada framebuffer
0×0 bo'ladi (Windows'da shunday chiqdi), `std::max(..., 1)` nolga bo'linishdan saqlaydi.

## 2. Uniform: hamma uchun bitta qiymat

Attribute har bir vertex yoki instance uchun o'z qiymatini oladi. Proyeksiya va zarracha diametri esa hamma uchun bir xil,
ularni har zarracha bilan saqlash isrof. **Uniform** shu uchun bor: bitta chizish buyrug'i davomida o'zgarmaydigan,
C++ dan beriladigan qiymat.

```glsl
uniform vec2 u_world_to_clip;      // shader'da
```

```cpp
GLint loc = glGetUniformLocation(program, "u_world_to_clip");   // nomdan raqam, bir marta
glUniform2f(loc, x, y);                                         // qiymatni berish
```

OpenGL 4.3 da raqamni shader'ning o'zida `layout(location = N)` bilan yozish mumkin, bizda 4.1, shuning uchun nom bo'yicha
so'raymiz. `2f` nomdagi qo'shimcha: 2 ta `float`.

Uchta nozik joy:

- `glUniform*` **hozir ishlatilayotgan** program'ga yozadi. Hech qaysi program ishlatilmayotgan bo'lsa, `GL_INVALID_OPERATION`.
  Shuning uchun avval `glUseProgram`, keyin `glUniform*`.
- Qiymat program ichida saqlanadi va keyingi `glUniform*`gacha turadi.
- Nom topilmasa `glGetUniformLocation` −1 qaytaradi, `glUniform*(-1, ...)` esa **jimgina e'tiborsiz qoldiriladi**. Compiler
  shader'da ishlatilmagan uniform'ni olib tashlaydi, bunda nom to'g'ri bo'lsa ham −1 chiqadi. Natija: bo'sh ekran va hech
  qanday xabar. Shuning uchun kichik `uniform_location` funksiyasini yozamiz, u terminalga xabar chiqaradi.

## 3. Zarracha shakli: quad va `gl_VertexID`

GPU uchburchaklarni chizadi, doirani emas. Eng sodda yo'l: har bir zarrachani ikki uchburchakdan iborat kvadrat (**quad**)
qilib chizamiz, doirani esa fragment shader'da kesib olamiz. Ko'pburchakli "doira" mesh yasash ham mumkin edi, lekin har
zarrachaga o'nlab vertex ketardi va cheti baribir qirrali bo'lardi.

Quadning oltita burchagi hamma zarrachada bir xil, ularni VBO'ga yozish shart emas. Vertex shader'da tayyor **`gl_VertexID`**
bor: hozirgi vertexning raqami. U jadvaldan burchakni tanlaydi:

```
5 (-1, 1) ─────── 2, 4 (1, 1)
    │            ╱ │
    │         ╱    │
    │      ╱       │
    │   ╱          │
0, 3 (-1,-1) ───── 1 (1,-1)
```

Dunyodagi o'rin shunday hisoblanadi:

```glsl
vec2 world = in_position + corner * 0.5 * u_diameter;
```

`corner` −1 dan 1 gacha, `0.5 * u_diameter` bilan ko'paytirilsa quadning yarim o'lchami chiqadi. `v_local = corner` fragment
shader'ga boradi va interpolation orqali har bir nuqtaning quad ichidagi koordinatasini beradi: markazda (0, 0), chetlarda ±1.

## 4. Instancing: bitta buyruq, minglab nusxa

Hammadan oddiy yo'l har zarrachani alohida chizish:

```cpp
for (const Particle& p : particles) {
    glUniform2f(loc_position, p.x, p.y);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
```

Har bir draw call drayver ichida CPU ishi: holatni tekshirish, buyruqni navbatga qo'yish. Zarracha ko'paysa, GPU emas, CPU
tiqilib qoladi. **Instancing** bitta buyruq bilan "shu 6 ta vertexni N marta chiz" deydi:

```cpp
glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particle_count);
```

Parametrlar `glDrawArrays`dagidek, faqat oxirida nusxalar soni qo'shilgan.

Har bir nusxa o'z o'rnini qayerdan biladi? **Divisor** orqali:

```cpp
glVertexAttribDivisor(0, 1);
```

Bu "0-attribute qanchalik tez yuradi" degani. Divisor 0 (odatiy): har vertexda keyingi element. Divisor 1: har nusxada
keyingi element.

```
VBO:         P0  P1  P2  P3  P4  P5  P6 ...

divisor 0:   vertex 0 → P0, vertex 1 → P1, ... vertex 5 → P5   (hamma nusxada bir xil)

divisor 1:   nusxa 0: oltala vertex → P0
             nusxa 1: oltala vertex → P1
             nusxa 2: oltala vertex → P2
```

02-darsdagi `struct` va `glVertexAttribPointer` aynan o'zi qoladi, baytlar joylashuvi o'zgarmaydi, faqat qadam
o'zgaradi. Divisor ham VAO ichida saqlanadi.

O'lchov natijasi (RTX 3060, i5-12400F, vsync o'chiq, 1280×720). O'ng ustunda har zarrachaga ikkita `glUniform*` va bitta
`glDrawArrays` chaqiriladi:

| Zarracha | Instancing | Alohida draw call |
|---|---|---|
| 4 536 | 9710 FPS | 1099 FPS |
| 40 000 | 7243 FPS | 132 FPS |
| 250 000 | 1606 FPS | 23 FPS |
| 1 000 000 | 407 FPS | o'lchanmadi |

Vertex shader baribir 6·N marta ishlaydi, instancing GPU ishini kamaytirmaydi. U draw call'lar sonini N dan 1 ga tushiradi,
ya'ni CPU tomonidagi ishni olib tashlaydi.

## 5. Silliq doira: `smoothstep` va `fwidth`

Fragment shader'da `dist = length(v_local)` markazdan masofa, doira esa `dist < 1` bo'lgan nuqtalar.

Eng sodda variant: `alpha = dist < 1.0 ? 1.0 : 0.0`. Har bir piksel yo to'liq ichkarida, yo to'liq tashqarida bo'ladi va
chet zinapoya bo'lib ko'rinadi (*aliasing*). Aslida chet pikselning bir qismini qoplaydi, shuning uchun bu piksel oraliq
shaffoflik olishi kerak.

**`smoothstep(a, b, x)`**: `x ≤ a` bo'lsa 0, `x ≥ b` bo'lsa 1, orasida silliq o'tadi:

```
t = clamp((x - a) / (b - a), 0, 1)
smoothstep = t * t * (3 - 2 * t)
```

O'tish zonasi kengligi **bir piksel** bo'lishi kerak. Lekin bir piksel `v_local` birliklarida qancha? Bu zarracha ekranda
qancha joy egallashiga bog'liq: radiusi 3 piksel bo'lsa, bir piksel 0.33 ga teng, radiusi 30 piksel bo'lsa 0.033 ga. Retina
ekranda yana boshqacha.

**`fwidth`** shuni o'lchaydi. GPU fragmentlarni 2×2 guruhlarda hisoblaydi, shuning uchun qo'shni piksellardagi qiymatni biladi
va ayirmasini ola oladi: `fwidth(d) = abs(dFdx(d)) + abs(dFdy(d))`, ya'ni "`d` keyingi pikselga o'tganda qanchaga o'zgaradi".
Bu faqat fragment shader'da ishlaydi.

```glsl
float dist = length(v_local);
float pixel = max(fwidth(dist), 0.0001);
float alpha = 1.0 - smoothstep(1.0 - pixel, 1.0, dist);
```

Ikki tafsilot:

- **Nega oraliq `[1 - pixel, 1]`.** Quad `dist = 1` chizig'ida, tomonlarning o'rtasida tugaydi, undan tashqarida fragment
  yo'q. Alpha aynan o'sha yerda 0 ga yetishi kerak. Keng tarqalgan `[1 - pixel, 1 + pixel]` variantida quad cheti alpha 0.5
  da kesiladi va doiraning to'rt tomonida tekis chiziq ko'rinadi (sinab ko'rdim).
- **Nega `max(..., 0.0001)`.** GLSL'da `edge0 >= edge1` bo'lsa `smoothstep` natijasi aniqlanmagan. Doira markazi 2×2 guruhning
  o'rtasiga aniq tushsa to'rtala `dist` teng bo'lib, `fwidth` nolga teng chiqishi mumkin.

## 6. Blending

Fragment shader `alpha` chiqaradi, lekin OpenGL odatda uni shaffoflik deb ishlatmaydi: rang framebuffer'ga to'g'ridan-to'g'ri
yoziladi va biz kvadratlarni ko'ramiz. **Blending** yoqilsa, yangi rang (*source*) ekrandagi rang (*destination*) bilan
aralashtiriladi:

```
natija = src.rgb * src.a + dst.rgb * (1 - src.a)
```

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

`alpha = 1` bo'lsa zarracha rangi, `alpha = 0` bo'lsa fon qoladi, orasida aralashma. Keyingi nusxa oldingisining ustiga
tushadi, tartib shu.

## 7. Kodni yozamiz

### 7.1. Eski shader'lar

`shaders/triangle.vert` va `shaders/triangle.frag` endi ishlatilmaydi, ularni o'chiring. Git tarixida saqlanib qoladi.

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
    X(PFNGLDRAWARRAYSPROC, glDrawArrays)                               \
    X(PFNGLENABLEPROC, glEnable)                                       \
    X(PFNGLBLENDFUNCPROC, glBlendFunc)                                 \
    X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation)               \
    X(PFNGLUNIFORM1FPROC, glUniform1f)                                 \
    X(PFNGLUNIFORM2FPROC, glUniform2f)                                 \
    X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor)             \
    X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced)
```

Yangi yettitasi OpenGL 4.1 da bor: `glDrawArraysInstanced` 3.1 dan, `glVertexAttribDivisor` 3.3 dan.

### 7.3. `shaders/particle2d.vert`

```glsl
#version 410 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

uniform vec2 u_world_to_clip;
uniform float u_diameter;

out vec2 v_local;
out vec3 v_color;

const vec2 CORNERS[6] = vec2[6](
    vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0)
);

void main() {
    vec2 corner = CORNERS[gl_VertexID];
    vec2 world = in_position + corner * 0.5 * u_diameter;
    gl_Position = vec4(world * u_world_to_clip, 0.0, 1.0);

    v_local = corner;
    v_color = in_color;
}
```

`world * u_world_to_clip`: GLSL'da ikki `vec2`ni ko'paytirish komponentma-komponent bo'ladi, ya'ni `x` `x` ga, `y` `y` ga.
Bu 1-bo'limdagi formulaning o'zi. `in_position` va `in_color` endi har nusxa uchun bitta qiymat oladi.

### 7.4. `shaders/particle2d.frag`

```glsl
#version 410 core

in vec2 v_local;
in vec3 v_color;

layout(location = 0) out vec4 out_color;

void main() {
    float dist = length(v_local);
    float pixel = max(fwidth(dist), 0.0001);
    float alpha = 1.0 - smoothstep(1.0 - pixel, 1.0, dist);
    out_color = vec4(v_color, alpha);
}
```

### 7.5. `src/shader.hpp`

`create_program` e'lonining tagiga bitta qator qo'shing:

```cpp
GLint uniform_location(GLuint program, const char* name);
```

### 7.6. `src/shader.cpp`

Fayl boshida `#include <cstdio>` borligiga ishonch hosil qiling (`fprintf` shundan), oxiriga shuni qo'shing:

```cpp
GLint uniform_location(GLuint program, const char* name) {
    const GLint location = glGetUniformLocation(program, name);
    if (location == -1) {
        std::fprintf(stderr, "Uniform topilmadi: %s\n", name);
    }
    return location;
}
```

### 7.7. `src/main.cpp` (to'liq almashtiring)

```cpp
#include "gl.hpp"
#include "shader.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

struct Particle {
    float x, y;
    float r, g, b;
};

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

static const char* gl_string(GLenum name) {
    return reinterpret_cast<const char*>(glGetString(name));
}

static std::vector<Particle> make_grid(int nx, int ny, float width, float height) {
    std::vector<Particle> particles;
    particles.reserve(nx * ny);

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const float tx = i / (nx - 1.0f);
            const float ty = j / (ny - 1.0f);

            Particle p;
            p.x = (tx - 0.5f) * width;
            p.y = (ty - 0.5f) * height;
            p.r = 0.2f + 0.8f * tx;
            p.g = 0.2f + 0.8f * ty;
            p.b = 1.0f - 0.7f * tx;
            particles.push_back(p);
        }
    }
    return particles;
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

    GLuint program = create_program("particle2d.vert", "particle2d.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLint loc_world_to_clip = uniform_location(program, "u_world_to_clip");
    const GLint loc_diameter = uniform_location(program, "u_diameter");

    const std::vector<Particle> particles = make_grid(81, 56, 6.42f, 4.39f);
    const int particle_count = static_cast<int>(particles.size());

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Particle),
                          reinterpret_cast<const void*>(offsetof(Particle, x)));
    glVertexAttribDivisor(0, 1);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle),
                          reinterpret_cast<const void*>(offsetof(Particle, r)));
    glVertexAttribDivisor(1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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

        const float half_height = 5.0f;
        const float aspect = static_cast<float>(std::max(fb_width, 1)) / std::max(fb_height, 1);
        const float half_width = half_height * aspect;

        glUseProgram(program);
        glUniform2f(loc_world_to_clip, 1.0f / half_width, 1.0f / half_height);
        glUniform1f(loc_diameter, 0.08f);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particle_count);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        const double now = glfwGetTime();
        if (now - fps_time >= 0.5) {
            int win_width = 0;
            int win_height = 0;
            glfwGetWindowSize(window, &win_width, &win_height);

            char title[128];
            std::snprintf(title, sizeof(title), "Fluid Sim | %.0f FPS | %d zarracha | oyna %dx%d | framebuffer %dx%d",
                          fps_frames / (now - fps_time), particle_count, win_width, win_height, fb_width, fb_height);
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

02-darsdan farqlari:

**`struct Particle`** o'sha `Vertex`ning o'zi, 20 bayt. Endi u bitta vertexni emas, bitta zarrachani tasvirlaydi.

**`make_grid`** zarrachalarni tekis to'r qilib joylaydi. `tx`, `ty` 0 dan 1 gacha yuradi, shu qiymatlardan o'rin va rang
hisoblanadi. 81×56 va 6.42×4.39 sonlari keyingi darslardagi birinchi sahnadan olingan, suyuqlik aynan shu to'rdan boshlanadi.

**`reserve`** vektor uchun xotirani oldindan oladi, `push_back` paytida qayta-qayta ko'chirish bo'lmaydi.

**`uniform_location`** bir marta, sikldan oldin chaqiriladi: raqam program link qilingandan keyin o'zgarmaydi.

**`glBufferData`** `particles.size() * sizeof(Particle)` bayt nusxalaydi. `particles.data()` vektor ichidagi massivning boshi.
`GL_STATIC_DRAW`, chunki hozircha zarrachalar qimirlamaydi.

**`glVertexAttribDivisor(0, 1)` va `(1, 1)`** ikkala attribute'ni ham "har nusxada keyingi element" qiladi.

**`glEnable(GL_BLEND)`** bir marta yoqiladi va shu holatda qoladi. OpenGL holat mashinasi: yoqilgan narsa o'chirilguncha turadi.

**Kadr ichida** `glUniform2f` har safar chaqiriladi, chunki oyna o'lchami istalgan payt o'zgarishi mumkin. `half_height = 5` kelgusi
sahnaning ko'rinadigan balandligi, `0.08` zarracha diametri.

### 7.8. Yig'ish va ishga tushirish

```
cmake --build build
.\build\fluid_sim.exe      # Windows (PowerShell)
./build/fluid_sim          # macOS
```

## 8. Ishlaganini qanday tekshirasiz

**Windows.** Bu natijani o'zim ko'rdim:

- To'q fonda markazda 81×56 = 4536 ta kichik doiradan iborat to'r: chap past ko'k, chap tepa moviy, o'ng past qizil, o'ng
  tepa sariq.
- 1280×720 da to'r 468×322 pikselni egallaydi. Hisob ham shu: (6.42 + 0.08) × 72 = 468, (4.39 + 0.08) × 72 = 322.
- Oyna shaklini o'zgartirsangiz doiralar doiraligicha qoladi: 900×900 oynada to'r 584×402 piksel (1 birlik 90 piksel).
- Terminalda `OpenGL xatosi` yoki `Uniform topilmadi` qatorlari yo'q, sarlavhada 165 FPS va `4536 zarracha`.

**macOS.** Buni o'zim ishga tushira olmadim, kutilayotgan natija: xuddi shu to'r, oynaning xuddi shu joyida. Retina ekranda
sarlavhadagi framebuffer oynadan ikki baravar katta bo'ladi va doira chetlari aniqroq ko'rinadi, chunki `fwidth` piksel
o'lchamiga o'zi moslashadi.

**Nima buzilsa nima bo'ladi.** Quyidagilarning hammasini RTX 3060 da sinadim:

| O'zgarish | Natija | Xabar bormi |
|---|---|---|
| `glVertexAttribDivisor` qatorlari yo'q | ekranda chap past burchakda kichik dog', 12×5 piksel: hamma nusxa bir xil 6 ta elementni o'qiydi | yo'q |
| `glEnable(GL_BLEND)` yo'q | doiralar o'rniga kvadratlar | yo'q |
| uniform nomida xato (`"u_world_to_clp"`) | ekran bo'sh, FPS joyida | `Uniform topilmadi: u_world_to_clp` |
| shader `u_diameter`ni ishlatmaydi | nom to'g'ri bo'lsa ham −1 | `Uniform topilmadi: u_diameter` |
| `half_width = half_height` | to'r eniga 1.78 baravar cho'zilgan, doiralar ellips | yo'q |
| `glUniform*` `glUseProgram`dan oldin | birinchi kadrda xato, keyin ishlaydi (program bog'liq qolgani uchun) | bir marta `GL_INVALID_OPERATION` |
| `alpha = dist < 1.0 ? 1.0 : 0.0` | chet zinapoya | yo'q |
| `pixel = 0.05` (o'zgarmas) | 43 pikselli doirada deyarli to'g'ri, 6 pikselli zarrachada chet silliqlanmaydi va doiralar bir-biriga yopishadi | yo'q |
| `smoothstep(1.0 - pixel, 1.0 + pixel, dist)` | doiraning to'rt tomonida tekis kesilgan chiziq | yo'q |

`glUseProgram` qatorida ayniqsa diqqat qiling: birinchi kadrdagi xato terminalda bir marta chiqib o'tib ketadi, rasm esa to'g'ri
ko'rinadi. Xabarni o'qimasangiz xatoni sezmaysiz.

## 9. Git va Mac

Windows'da:

```
git add .
git commit -m "Lesson 03: world coordinates, instancing, smooth circles"
git push
```

Mac'da:

```
git pull
cmake --build build
./build/fluid_sim
```

## 10. Ixtiyoriy tajribalar

1. `main.cpp`da `make_grid(9, 6, 6.42f, 4.39f)` va `glUniform1f(loc_diameter, 0.6f)` qiling. Doiralar katta bo'ladi va silliq
   chetini yaxshi ko'rasiz. Keyin `glEnable(GL_BLEND)` qatorini vaqtincha o'chirib solishtiring.
2. `make_grid(1000, 1000, 6.42f, 4.39f)` va `glfwSwapInterval(0)` qiling: million zarracha. Windows'da 407 FPS chiqdi, Mac'da
   qancha chiqishini yozing.

## Keyingi dars

04-darsda zarrachalar jonlanadi: tezlik, gravitatsiya, timestep, devorlar bilan to'qnashuv va har kadrda yangilanadigan VBO.

Shu dars tugagach yozing. Xohlasangiz, kodingizni ko'rib chiqaman.
