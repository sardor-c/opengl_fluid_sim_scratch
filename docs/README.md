# OpenGL 4.1 bilan suyuqlik simulyatsiyasi: kurs mundarijasi

Bu kursda SPH (Smoothed Particle Hydrodynamics) suyuqlik simulyatsiyasini noldan quramiz. Kodni o'zingiz
yozasiz, har bir qadam bitta darsda tushuntiriladi. Kod Windows'da ham, Mac'da ham bir xil ishlaydi,
shuning uchun OpenGL 4.1 core'dan chiqmaymiz (macOS qo'llaydigan eng yangi versiya).

Holat: **bajarildi** (ikkala kompyuterda ishladi), **yozildi** (dars tayyor), **reja** (hali yozilmagan).
Reja kurs davomida o'zgarishi mumkin.

## I qism: muhit va OpenGL asoslari

| # | Dars | Holat |
|---|---|---|
| 00 | [Muhit va loyiha: ikki kompyuterda bo'sh oyna](00-muhit-va-loyiha.md) | bajarildi |
| 01 | [OpenGL konteksti: 4.1 core, funksiyalarni yuklash, asosiy sikl](01-opengl-konteksti.md) | bajarildi |
| 02 | [Birinchi uchburchak: shader, VAO, VBO](02-birinchi-uchburchak.md) | bajarildi |
| 03 | [Ko'p zarracha: dunyo koordinatalari, instancing, silliq doira](03-kop-zarracha.md) | bajarildi |

## II qism: CPU'da 2D SPH

| # | Dars | Holat |
|---|---|---|
| 04 | [Harakat: gravitatsiya, timestep, devorlar](04-harakat.md) | yozildi |
| 05 | Zichlik: SPH g'oyasi, smoothing kernel | reja |
| 06 | Bosim: birinchi suyuqlik | reja |
| 07 | Yopishqoqlik va sichqoncha | reja |
| 08 | Tez qo'shni qidirish: counting sort | reja |
| 09 | Ko'p oqim (ixtiyoriy) | reja |
| 10 | Sahna: fon, SDF, to'siqlar | reja |

## III qism: GPU'da simulyatsiya (OpenGL 4.1 doirasida GPGPU)

| # | Dars | Holat |
|---|---|---|
| 11 | Ma'lumot teksturada | reja |
| 12 | Teksturaga chizish: FBO, MRT, ping-pong | reja |
| 13 | GPU'da zichlik va bosim | reja |
| 14 | GPU'da saralash: bitonic sort | reja |

## IV qism: 3D

| # | Dars | Holat |
|---|---|---|
| 15 | 3D kamera va depth buffer | reja |
| 16 | 3D zarrachalar | reja |
| 17 | Yakun | reja |
