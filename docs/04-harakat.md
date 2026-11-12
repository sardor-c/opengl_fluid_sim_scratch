# 04. Harakat: tezlik, gravitatsiya, timestep va devorlar

## Bu qadamda nima qo'shamiz va nega aynan hozir

03-darsda 4536 ta doira joyida qimirlamay turardi. Endi ularni harakatlantiramiz.

Keyingi darslardagi SPH (suyuqlikni zarrachalar bilan hisoblash usuli) aslida bitta narsani qo'shadi: zarrachaga ta'sir qiladigan
kuchni. Qolgani, ya'ni kuch zarrachani qanday siljitishi, vaqt qanday hisoblanishi, devorlar va har kadrda ma'lumotni GPU'ga
yuborish SPH'ga bog'liq emas. Shularni hozir, yagona kuch gravitatsiya bo'lganda quramiz, chunki bu holatda natijani oddiy hisob
bilan tekshirish mumkin.

**Bu darsni o'qish uchun fizika yoki matematika bilish shart emas.** Har bir tushuncha noldan tushuntiriladi: avval kundalik
misol, keyin kichik sonlar bilan qo'lda hisob, oxirida kodda qayerda turgani. Har bir tushuncha oldingisiga tayanadi, shuning uchun
bo'limlarni tartib bilan o'qing. Dars oxirida qisqa lug'at bor.

Dars oxirida blok pastga tushadi, polga urilib sakraydi, qatorlar bir-biridan o'tib ketadi va taxminan 40 soniyada hamma
zarracha polda bitta chiziqqa yig'iladi. Bu xato emas: zarrachalar hali bir-birini sezmaydi. 05 va 06-darslar aynan
shuni tuzatadi.

Loyihada shular o'zgaradi (shader'lar va `gl.hpp`ga tegmaymiz, yangi GL funksiyasi kerak emas):

```
opengl_fluid_sim_scratch/
├── CMakeLists.txt          (bitta qator qo'shiladi)
└── src/
    ├── sim2d.hpp           (yangi)
    ├── sim2d.cpp           (yangi)
    └── main.cpp            (o'zgaradi)
```

Simulyatsiya alohida faylga chiqadi va unda bironta OpenGL chaqiruvi bo'lmaydi. Fizika oddiy C++ massivlar ustida ishlaydi,
`main.cpp` esa natijani chizadi. Bu ajratish III qismda foyda beradi: o'sha yerda simulyatsiyani GPU'ga ko'chiramiz va CPU
versiyasi bilan raqamma-raqam solishtiramiz.

## 1. O'rin: zarracha qayerda turibdi

Kimgadir yo'l ko'rsatayotganingizni tasavvur qiling: "maydondan 3 qadam o'ngga, 2 qadam yuqoriga yuring". Maydon boshlang'ich nuqta,
ikkita son esa joyni to'liq aytadi. Zarracha o'rni ham shunday beriladi.

**Koordinata.** Dunyomizning markazi `(0, 0)` nuqta. Har qanday joy ikki son bilan aytiladi:

- `x`: markazdan o'ngga qancha. Manfiy bo'lsa, chapga.
- `y`: markazdan yuqoriga qancha. Manfiy bo'lsa, pastga.

Manfiy son bu yerda "teskari tomonga" degan ma'noni beradi. `y = -4.65` degani "markazdan 4.65 birlik pastda".

Zarrachalarimiz yuradigan quti eni 17.1, bo'yi 9.3 birlik, markazi nolda. Demak markazdan har bir devorgacha o'lchamning yarmi:
`17.1 / 2 = 8.55` va `9.3 / 2 = 4.65`.

```
                         y
                         ▲
  (-8.55, 4.65) ┌────────┼────────┐ (8.55, 4.65)
                │        │        │
                │        │        │
          ──────┼──── (0, 0) ─────┼──────► x
                │        │        │
                │        │        │
 (-8.55, -4.65) └────────┼────────┘ (8.55, -4.65)
                       pol
```

"Birlik" metr ham, piksel ham emas, sahnamizning o'z o'lchovi. 03-darsda ko'rganimizdek, 1280 x 720 oynada 1 birlik 72 pikselga teng.

**Vektor.** `x` va `y` doim birga yuradi, shuning uchun ularni bitta narsa deb qaraymiz va `(x, y)` deb yozamiz. Bunday juftlikni
**vektor** deyiladi. Uni strelka deb tasavvur qilish qulay: `(3, 2)` "o'ngga 3, yuqoriga 2 boradigan strelka". Kodda bu `struct Vec2`.

Vektor bilan ikkita amal kerak bo'ladi:

- **Vektorni songa ko'paytirish.** Ikkala sonni ham ko'paytiramiz: `(3, 2) * 2 = (6, 4)`. Strelka yo'nalishi o'sha, uzunligi ikki
  baravar. `(3, 2) * 0.5 = (1.5, 1)` esa ikki baravar qisqa strelka.
- **Vektorlarni qo'shish.** `x` ni `x` ga, `y` ni `y` ga qo'shamiz: `(3, 2) + (1, -5) = (4, -3)`. Bu "avval birinchi strelka bo'ylab,
  keyin ikkinchisi bo'ylab yurish" degani.

Kodimizdagi ikkita `operator` aynan shu amallar. `a += b` esa "`a` ga `b` ni qo'shib, natijani `a` da qoldir" degani.

## 2. Tezlik: o'rin qanchalik tez o'zgaradi

Mashina soatiga 60 km yurayotgan bo'lsa, 1 soatda 60 km, 2 soatda 120 km yo'l bosadi. Ya'ni:

```
bosib o'tilgan yo'l = tezlik × vaqt
yangi o'rin = eski o'rin + tezlik × vaqt
```

Bizda vaqt soniyada, masofa birlikda, tezlik esa "bir soniyada necha birlik".

**Tezlik ham vektor** (velocity). Kundalik gapda "tezlik" faqat kattalikni aytadi, fizikada esa yo'nalish ham kiradi. `(2, -1)` degani
"har soniyada o'ngga 2 birlik va pastga 1 birlik". `(0, -5)` "to'g'ri pastga, soniyada 5 birlik".

Misol. Zarracha `(1, 3)` nuqtada, tezligi `(2, -1)`. 0.5 soniyadan keyin qayerda bo'ladi?

```
tezlik × vaqt  = (2, -1) * 0.5  = (1, -0.5)
yangi o'rin    = (1, 3) + (1, -0.5) = (2, 2.5)
```

Kodda shu hisobning o'zi turibdi (`dt` "qancha vaqt o'tdi" degani, uni 5-bo'limda batafsil ko'ramiz):

```cpp
sim.positions[i] += sim.velocities[i] * dt;
```

## 3. Tezlanish: tezlik qanchalik tez o'zgaradi

Mashinada gazni bosdingiz. Spidometr har soniyada shunday o'zgaradi: 0, 10, 20, 30 km/soat. Mana shu "har soniyada 10 ga oshyapti"
**tezlanish** (acceleration) deyiladi. Tormoz ham tezlanish, faqat yo'nalishi teskari: tezlikni kamaytiradi.

Demak zanjir shunday:

```
tezlanish  ── o'zgartiradi ──►  tezlik  ── o'zgartiradi ──►  o'rin
```

Formulasi 2-bo'limdagining aynan o'zi, faqat harflar boshqa:

```
yangi tezlik = eski tezlik + tezlanish × vaqt
```

Bizning tezlanish `(0, -12)`: har soniyada tezlik pastga qarab 12 ga oshadi. Tinch turgan zarracha 0.5 soniyadan keyin `(0, -6)`,
1 soniyadan keyin `(0, -12)` tezlikka ega bo'ladi.

Kodda:

```cpp
velocity += acceleration * dt;
```

Haqiqiy bitta qadamni sonlar bilan ko'ramiz. 165 Hz monitorda `dt = 0.00202` soniya chiqadi (nega, 7-bo'limda ko'ramiz). Zarrachaning
tezligi hozir `(0, -3)` bo'lsin:

```
tezlanish × dt  = (0, -12) * 0.00202     = (0, -0.0242)
yangi tezlik    = (0, -3) + (0, -0.0242) = (0, -3.0242)
o'rin o'zgarishi: -3.0242 * 0.00202      = -0.0061
```

Bitta qadamda o'zgarish juda kichik, lekin u soniyasiga yuzlab marta takrorlanadi.

## 4. Kuch, massa va gravitatsiya

**Kuch** (force) bu narsani itarish yoki tortish. Muhim nuqta: kuch tezlikni bermaydi, tezlikni **o'zgartiradi**. Muz ustida turgan shaybani
turtsangiz, u yura boshlaydi. Turtishni to'xtatsangiz ham yuraveradi, chunki kuch yo'q bo'lsa, tezlik o'zgarmaydi. Oxir-oqibat to'xtashining
sababi muzning ishqalanishi, bu ham kuch.

**Massa** (mass) narsani tezlashtirish qanchalik qiyinligini bildiradi. Bo'sh aravani yengil turtish bilan tezlashtirasiz, yuk ortilgan aravani
xuddi shunday turtsangiz, u arang qimirlaydi.

Nyuton bu ikkisini bitta qoidaga bog'lagan:

```
tezlanish = kuch / massa
```

Kuch ikki baravar ko'p bo'lsa, tezlanish ikki baravar ko'p. Massa ikki baravar ko'p bo'lsa, tezlanish ikki baravar kam.

**Gravitatsiya** (og'irlik kuchi) Yerning hamma narsani pastga tortishi. Qiziq tomoni shundaki, Yer og'ir narsani kuchliroq tortadi: massasi
ikki baravar katta narsaga kuch ham ikki baravar katta. Formulaga qo'ysak:

```
massa 1:  tezlanish = 10 / 1 = 10
massa 2:  tezlanish = 20 / 2 = 10
```

Massa qisqarib ketdi. Shuning uchun havo xalaqit bermasa, og'ir va yengil narsa bir xil tezlikda tushadi. Bizga bu juda qulay:
kodda massa umuman yo'q, hamma zarrachaga bir xil tezlanish yoziladi.

Bu tezlanish `gravity = -12`. Minus "pastga" degani. Yerda bu son taxminan 9.8, lekin u metrlarda o'lchanadi, bizning birliklarimiz
boshqa. 12 asl Unity sahnasidagi qiymat.

**Holat** (state). Zarracha haqida saqlashimiz kerak bo'lgan narsa ikkita: o'rin va tezlik. Tezlanishni saqlamaymiz, u har qadamda
kuchdan qaytadan hisoblanadi. Shuning uchun `Simulation` ichida `positions` va `velocities` degan ikkita massiv bor. Hozircha yagona kuch
gravitatsiya, 06-darsda esa zarrachalarni bir-biridan itaradigan bosim qo'shiladi.

## 5. Vaqtni qadamlarga bo'lish

Muammo shunda: "yangi o'rin = eski o'rin + tezlik × vaqt" formulasi faqat tezlik o'zgarmasa to'g'ri. Tushayotgan zarrachaning tezligi
esa har lahzada o'zgaradi. Kompyuter esa hisobni birma-bir bajaradi, "har lahza" degan narsani qila olmaydi.

Yechim multfilm yoki chetiga rasm chizilgan daftarga o'xshaydi. Harakat alohida rasmlardan tashkil topadi, ular tez almashganda
uzluksiz ko'rinadi. Biz ham vaqtni kichik bo'laklarga bo'lamiz va **bitta bo'lak ichida tezlik o'zgarmaydi** deb hisoblaymiz.

Bo'lakning uzunligi **`dt`** deyiladi (delta time, "vaqtdagi kichik farq"), inglizcha nomi **timestep**. Bitta bo'lakni hisoblash bitta **qadam**.
Har qadamda ikki ish qilamiz:

```
1. tezlik = tezlik + tezlanish × dt
2. o'rin  = o'rin  + tezlik × dt
```

Bu usul **Euler integratsiyasi** deyiladi (Euler matematik, o'qilishi "Oyler"). "Integratsiya" bu yerda "kichik o'zgarishlarni ketma-ket qo'shib
borish" degani.

Qo'lda hisoblab ko'ramiz. Zarracha `y = 0` da tinch turibdi, tezlanish −12, `dt = 0.1` soniya:

| qadam | vaqt | tezlik | o'rin |
|---|---|---|---|
| 1 | 0.1 | 0 − 1.2 = −1.2 | 0 − 0.12 = −0.12 |
| 2 | 0.2 | −1.2 − 1.2 = −2.4 | −0.12 − 0.24 = −0.36 |
| 3 | 0.3 | −2.4 − 1.2 = −3.6 | −0.36 − 0.36 = −0.72 |
| 4 | 0.4 | −3.6 − 1.2 = −4.8 | −0.72 − 0.48 = −1.20 |
| 5 | 0.5 | −4.8 − 1.2 = −6.0 | −1.20 − 0.60 = −1.80 |

`−1.2` har qadamdagi `−12 × 0.1`. O'rin ustunidagi ikkinchi son esa `tezlik × 0.1`. Jadvalga qarang: tezlik har qadamda bir xil miqdorga
o'zgaradi, o'rin esa tobora katta bo'laklarga. Tushayotgan narsaning borgan sari tezlashishi aynan shu.

**To'g'ri javob qancha?** Bu sodda holatda buni aniq topish mumkin. 0.5 soniya ichida tezlik 0 dan 6 gacha tekis o'sadi, demak o'rtacha
tezlik `(0 + 6) / 2 = 3`. Yo'l `3 × 0.5 = 1.5`. Aniq javob −1.5, bizning jadvalda −1.80. Xato 0.30 chiqdi, chunki bo'lak ichida tezlik
o'zgarmas deb oldik.

`dt` ni kichraytirsak, xato kamayadi:

| `dt` | qadamlar soni | natija | xato |
|---|---|---|---|
| 0.1 | 5 | −1.800 | 0.300 |
| 0.05 | 10 | −1.650 | 0.150 |
| 0.01 | 50 | −1.530 | 0.030 |
| 0.001 | 500 | −1.503 | 0.003 |

`dt` 10 baravar kichik bo'lsa, xato ham 10 baravar kichik, lekin qadam 10 baravar ko'p, ya'ni kompyuterga ish ham ko'p.

Matematikada "bir narsa qanchalik tez o'zgaryapti" degan tushuncha **hosila** (derivative) deyiladi: tezlik o'rinning hosilasi, tezlanish tezlikning
hosilasi. Hozircha shu nomni eshitib qo'yish yetarli.

## 6. Ikki qatorning tartibi nega muhim

Qadamdagi ikki ishni teskari tartibda ham bajarish mumkin:

```
semi-implicit Euler (biz shuni ishlatamiz):  avval tezlik, keyin o'rin
explicit Euler:                              avval o'rin, keyin tezlik
```

Explicit usulda zarracha hali eski tezlik bilan yuradi, yangi tezlik keyingi qadamga qoladi. Semi-implicit usulda esa yangilangan tezlik
bilan yuradi. Farqi ikki qatorning tartibi, xolos, lekin oqibati juda katta bo'lishi mumkin.

**Erkin tushishda farq deyarli yo'q.** `dt = 1/180` bilan 0.5 soniya tushganda aniq javob −1.5, explicit −1.4833, semi-implicit −1.5167 beradi.
Biri ozgina orqada, ikkinchisi ozgina oldinda. `step_simulation` dagi ikki chaqiruvning o'rnini almashtirib, 60 soniya yurgizib ko'rdim: eng
baland nuqta ikkala tartibda ham bir xil chiqdi. Demak bu darsda farqni ko'zingiz bilan ko'rmaysiz.

**Kuch o'ringa bog'liq bo'lganda hammasi o'zgaradi.** Buni eng sodda misolda, prujinada ko'ramiz.

**Prujina.** Bir uchi mahkamlangan prujinaning ikkinchi uchiga zarracha ulangan deb tasavvur qiling. Cho'zsangiz, u orqaga tortadi. Siqsangiz,
itaradi. Qancha ko'p cho'zsangiz, shuncha kuchli tortadi. Prujinaning bo'sh holatini `x = 0` deb olsak:

```
tezlanish = -k × x
```

`k` prujinaning **qattiqligi** (stiffness): katta `k` qattiq prujina. Minus belgisi yo'nalishni to'g'rilaydi: `x` musbat bo'lsa (o'ngda),
tezlanish manfiy (chapga), `x` manfiy bo'lsa, tezlanish musbat (o'ngga). Ya'ni prujina zarrachani doim markazga qaytaradi.

Zarrachani cho'zib qo'yib yuborsak, u markazga keladi, tezligi bilan o'tib ketadi, narigi tomonda to'xtab, yana qaytadi. Bu **tebranish**, xuddi
belanchak kabi. Ishqalanish bo'lmasa, har safar aynan bir xil masofagacha boradi.

Prujina bizga nega kerak? 06-darsdagi bosim shunday ishlaydi: zarrachalar bir-biriga juda yaqinlashsa, bir-birini itaradi. Ya'ni suyuqlik
o'zini minglab kichik prujinadek tutadi. Integratsiya prujinada qanday ishlasa, suyuqlikda ham shunday ishlaydi.

Qo'lda hisoblaymiz: `k = 1`, zarracha `x = 1` da tinch turibdi, `dt = 0.5` (farq yaxshi ko'rinishi uchun atayin katta). Birinchi qadamda
tezlanish `−1 × 1 = −1`.

```
semi-implicit:  tezlik = 0 + (-1) * 0.5  = -0.5
                o'rin  = 1 + (-0.5) * 0.5 = 0.75

explicit:       o'rin  = 1 + 0 * 0.5     = 1       (hali eski tezlik, ya'ni 0 bilan yurdi)
                tezlik = 0 + (-1) * 0.5  = -0.5
```

Davomi:

| qadam | explicit: o'rin | explicit: tezlik | semi-implicit: o'rin | semi-implicit: tezlik |
|---|---|---|---|---|
| 0 | 1.000 | 0 | 1.000 | 0 |
| 1 | 1.000 | −0.500 | 0.750 | −0.500 |
| 2 | 0.750 | −1.000 | 0.312 | −0.875 |
| 3 | 0.250 | −1.375 | −0.203 | −1.031 |
| 4 | −0.438 | −1.500 | −0.668 | −0.930 |
| 5 | −1.188 | −1.281 | −0.966 | −0.596 |
| 6 | −1.828 | −0.688 | −1.022 | −0.113 |

1 dan boshlagan edik, demak narigi tomonda −1 atrofida to'xtashi kerak. Semi-implicit −1.02 ga bordi. Explicit esa allaqachon −1.83 da
va hali to'xtamagan. Xuddi shu hisobni davom ettirsak, zarracha har tomonda eng uzoq qayergacha borganini ko'ramiz:

| tebranish | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| explicit | 1.00 | 2.17 | 4.67 | 9.91 | 20.80 | 44.34 | 95.74 |
| semi-implicit | 1.00 | 1.02 | 1.03 | 1.03 | 1.02 | 1.01 | 1.03 |

Explicit usulda har safar ikki baravardan ko'proq kengayadi. Bu shunchaki noaniqlik emas, jismonan mumkin bo'lmagan natija. Sababini
energiya orqali tushunamiz.

**Energiya** bu narsada yig'ilgan "ish qilish zaxirasi". U ikki ko'rinishda bo'ladi: harakatlanayotgan narsada (*harakat energiyasi*) va cho'zilgan
prujinada yoki baland joyda turgan narsada (*yig'ilgan energiya*). Belanchakni eslang: eng tepada u bir lahza to'xtaydi, hamma energiya balandlikda.
Eng pastda eng tez, hamma energiya harakatda. Energiya bir ko'rinishdan ikkinchisiga o'tib turadi, lekin **yo'qdan paydo bo'lmaydi**. Ishqalanish
uni kamaytiradi, o'z-o'zidan esa hech qachon ko'paymaydi.

Tebranish kengaymoqda, demak energiya o'smoqda. Bunday bo'lishi mumkin emas, ya'ni bu hisobimizning xatosi.

Nega aynan explicit usul xato qiladi? Zarracha markazdan uzoqlashayotganda prujina uni sekinlatmoqchi. Explicit usulda zarracha avval hali
sekinlashmagan eski tezlik bilan yuradi, tormoz faqat keyin ishlaydi. Tormozni kech bosgan mashina uzoqroq borib to'xtaydi. Qaytishda ham
tezlik kech qo'shiladi, zarracha uzoqroqda ko'proq turib, prujinadan ortiqcha tezlik olib ketadi. Har qadamdagi kichik ortiqcha yig'ilib boradi.
Semi-implicit usulda avval tormoz, keyin yurish, shuning uchun ortiqcha yig'ilib bormaydi (jadvalda 1.00 atrofida qoladi).

`dt` kichik bo'lsa ham bu to'xtamaydi, faqat sekinlashadi. 10 soniyadan keyin tebranish boshlang'ichidan necha baravar katta bo'lganini
o'lchadim (ideal javob 1):

| `k` | `dt` | explicit | semi-implicit |
|---|---|---|---|
| 100 | 1/180 | 15 | 1.00 |
| 100 | 1/60 | 3 234 | 1.00 |
| 400 | 1/180 | 58 370 | 1.00 |

Shuning uchun ko'p zarracha simulyatsiyalari, jumladan asl Fluid-Sim ham, semi-implicit usuldan foydalanadi. Bizning kodda bu `apply_gravity`,
keyin `update_positions` tartibi.

**Barqarorlik** (stability). Semi-implicit usul ham cheksiz chidamli emas: qadam prujina uchun juda katta bo'lsa, u ham portlaydi. 1000 qadam ichida
zarracha borgan eng uzoq nuqta (boshida 1 edi):

| `k` | `dt` | eng uzoq nuqta |
|---|---|---|
| 100 | 0.100 | 1 |
| 100 | 0.199 | 10 |
| 100 | 0.201 | 88 xonali ulkan son |
| 10 000 | 0.0199 | 10 |
| 10 000 | 0.0201 | 88 xonali ulkan son |

Ikki narsaga e'tibor bering. Chegara keskin: 0.199 da hali yashaydi, 0.201 da butunlay buziladi. Prujina qattiqlashsa, ruxsat etilgan qadam kichrayadi.
Suyuqlikdagi bosim juda qattiq prujina, shuning uchun 06-darsda `dt` ni kichik tutishga majbur bo'lamiz. Simulyatsiyaning "portlashi" aynan shu hodisa.

## 7. `dt` qayerdan keladi

**Kadr** (frame) ekranga chizilgan bitta rasm. **FPS** (frames per second) soniyada nechta kadr chizilishini aytadi. **Hz** ham "soniyada necha marta"
degani: 165 Hz monitor soniyada 165 marta yangilanadi, bitta kadrga `1 / 165 = 0.00606` soniya to'g'ri keladi. Mac'dagi 60 Hz ekranda
`1 / 60 = 0.01667` soniya.

Eng oddiy fikr har kadrda bir xil `dt` ishlatish. Lekin kompyuterlar har xil tezlikda chizadi, simulyatsiya vaqti esa haqiqiy vaqtga mos kelishi kerak.
Blokning pastki qatori polgacha 3.1 birlik tushadi. Hisob bo'yicha bu 0.719 soniya oladi: shu vaqtda tezlik `12 × 0.719 = 8.63` ga yetadi, o'rtacha
tezlik 4.31, yo'l esa `4.31 × 0.719 = 3.1`. Ikki usulni o'lchadim:

| FPS | `dt` kadr vaqtidan olinadi | `dt = 1/180` qotirilgan |
|---|---|---|
| 30 | 1.433 s | 1.433 s |
| 60 | 0.717 s | 0.717 s |
| 120 | 0.725 s | 0.358 s |
| 165 | 0.721 s | 0.261 s |
| 240 | 0.721 s | 0.179 s |

Qotirilgan `dt` da 165 Hz monitorda hamma narsa 2.75 baravar tez ketadi. Yechim: har kadr qancha davom etganini o'lchash.
`glfwGetTime()` sekundomerga o'xshaydi: dastur boshlanganidan beri necha soniya o'tganini beradi. Oldingi kadrda 2.000, hozir 2.006 ko'rsatsa,
kadr 0.006 soniya davom etgan bo'ladi.

```cpp
const double now = glfwGetTime();
const float frame_time = static_cast<float>(now - last_time);
last_time = now;

const float dt = std::min(frame_time, settings.max_frame_time) / settings.iterations_per_frame;
for (int i = 0; i < settings.iterations_per_frame; i++) {
    step_simulation(sim, settings, dt);
}
```

Bu yerda ikkita muhim g'oya bor.

**Kadr vaqtini cheklash.** `std::min(a, b)` ikki sondan kichigini qaytaradi. Nega kerak? GLFW hujjatiga ko'ra, ba'zi platformalarda oynani sudrash
yoki o'lchamini o'zgartirish paytida `glfwPollEvents` bloklanadi. Debugger to'xtashi yoki tizim qotishi ham shunday: bitta kadr 2 soniya davom etishi
mumkin. O'rtadagi zarrachani shunday kadrga qo'yib ko'rdim: cheklovsiz u `y = 0.682` dan bitta kadrda polga "teleport" bo'ldi, cheklov bilan esa `y = 0.668`
ga tushdi. 6-bo'limdagi jadvalni eslang: juda katta `dt` suyuqlikni portlatadi. Shuning uchun kadr vaqti `1/60` dan oshmaydi. Kompyuter ulgurmasa,
simulyatsiya sekinlashadi, lekin buzilmaydi (jadvaldagi 30 FPS qatori: ikki baravar sekin).

**Bir kadrda bir necha qadam** (iterations). Kadr vaqtini 3 ga bo'lib, simulyatsiyani 3 marta yurgizamiz. Chizish esa bir marta. Yana 5 va 6-bo'limlarga
qarang: qadam qancha kichik bo'lsa, xato shuncha kam va portlash xavfi shuncha uzoq. 60 Hz da bitta qadam `1/60 / 3 = 1/180` soniya, 165 Hz da
`1/165 / 3 = 1/495 = 0.00202` soniya. 3-bo'limdagi son shu yerdan chiqqan edi.

Ikki kompyuterda `dt` har xil bo'lgani uchun natija bitma-bit bir xil bo'lmaydi, lekin harakat bir xil ko'rinadi.

## 8. Devorlar

Zarracha polga yetganda, u bitta qadamda pol ostiga o'tib ketadi:

```
      ●   qadamdan oldin, tezligi pastga
      │
──────┼──────  pol, y = -4.65
      ●        qadamdan keyin, y = -4.70
```

Ikki narsani to'g'rilaymiz:

1. **O'rinni** devorga qaytaramiz.
2. **Tezlikni** teskari qilamiz va biroz kamaytiramiz. Bu sakrash.

Kodda:

```cpp
if (std::fabs(position.y) >= half_height) {
    position.y = std::copysign(half_height, position.y);
    velocity.y *= -settings.collision_damping;
}
```

Har bir qismini sonlar bilan ko'ramiz. `y = -4.70`, tezlik `-3.0`, `half_height = 4.65`.

**`std::fabs`** sonning ishorasini olib tashlaydi (bunga *absolute value*, "mutlaq qiymat" deyiladi): `fabs(-4.70) = 4.70`, `fabs(4.70) = 4.70`. Natija "markazdan qancha
uzoqda" degani. `4.70 >= 4.65` to'g'ri, demak zarracha chegaradan chiqqan. Bitta tekshiruv ham pol, ham shift uchun ishlaydi.

**`std::copysign(a, b)`** `a` sonini oladi va unga `b` ning ishorasini beradi: `copysign(4.65, -4.70) = -4.65`. Zarracha pastdan chiqqan bo'lsa, polga, yuqoridan
chiqqan bo'lsa, shiftga qaytadi.

**`velocity.y *= -0.95`** "o'zini −0.95 ga ko'paytir" degani: `-3.0 * -0.95 = 2.85`. Ikki manfiy sonning ko'paytmasi musbat, demak tezlik pastga emas, endi yuqoriga.
Chap va o'ng devorlar uchun ham xuddi shu kod, faqat `x` bilan.

**Damping** (so'nish). 0.95 "tezlikning 95% qoladi, 5% yo'qoladi" degani. Haqiqatda ham devorga urilgan narsa energiyasining bir qismini issiqlik va tovushga beradi.

Tezlik 5% ga kamayadi, energiya esa ko'proq. Sababi, harakat energiyasi tezlikning **kvadratiga** bog'liq. Kvadrat bu sonni o'ziga ko'paytirish: `10 × 10 = 100`, `9.5 × 9.5 = 90.25`.
Demak tezlik 10 dan 9.5 ga tushsa, energiya 100 dan 90.25 ga, ya'ni 90.25% ga tushadi. Shu sababli tezligi ikki baravar katta mashinaning tormoz yo'li taxminan to'rt baravar uzun.

Urilgandan keyin harakat energiyasi balandlikka aylanadi, shuning uchun har sakrashning balandligi oldingisining taxminan 90% ini tashkil qilishi kerak. Haqiqiy kod bilan bitta
zarrachani tashlab, o'lchadim. Polga urilishdagi tezlik 13.43 edi, urilgandan keyin 12.76 (bu 95%):

| sakrash | boshida | 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|---|
| poldan balandlik | 7.52 | 6.77 | 6.10 | 5.48 | 4.94 | 4.45 |

Hamma zarracha polga yotishi uchun (poldan 0.02 birlikdan uzoq emas) ketadigan vaqt:

| `collision_damping` | vaqt |
|---|---|
| 1.0 | 60 soniyada ham yotmaydi |
| 0.95 | 39 s |
| 0.8 | 9.4 s |
| 0.5 | 3.2 s |
| 0 | 1.1 s |

Ikkala to'g'rilash ham zarur, bittasi yetmaydi. Ikkalasini ham alohida sinab ko'rdim:

- **Faqat tezlik teskari bo'lsa**, zarracha pol ostida qoladi. Keyingi qadamda u yuqoriga chiqishga ulgurmaydi, tezlik yana teskari bo'ladi va zarracha tuzoqqa tushadi.
  40 soniyadan keyin 4536 tasining hammasi pol ostida edi, eng chuquri 0.5 birlikda.
- **Faqat o'rin qaytarilsa**, rasm mutlaqo to'g'ri ko'rinadi: hamma polda yotibdi. Lekin tezlik o'sishda davom etadi va 10 soniyadan keyin 120 ga yetadi (`12 × 10`).
  Bu eng xavfli xato turi: ko'zga ko'rinmaydi, SPH'da esa kutilmagan portlash beradi.

Devor bilan zarracha **markazi** to'qnashadi, chetlari emas. Shuning uchun doira devorga diametrining yarmi (0.04 birlik, 3 piksel) kirib turadi. Asl loyiha ham shunday qiladi.

## 9. Ma'lumotni ajratish: ikkita VBO

03-darsda bitta `Particle` struct bor edi: o'rin va rang birga, bitta VBO'da. Endi ularning xulqi har xil: o'rin har kadrda
o'zgaradi, rang esa hech qachon. Birga saqlasak, har kadrda 20 baytdan yuboramiz va shundan 12 bayti behuda bo'ladi. Fizikaning
ham rangdan xabari bo'lishi shart emas.

Shuning uchun ma'lumot ikki buferga ajraladi:

```
position_vbo:   x0 y0 | x1 y1 | x2 y2 | ...            8 bayt, har kadrda yangilanadi
color_vbo:      r0 g0 b0 | r1 g1 b1 | r2 g2 b2 | ...   12 bayt, bir marta yoziladi
```

Simulyatsiya tomonida ham xuddi shu g'oya: bitta `Particle` massivi (**AoS**, array of structures) o'rniga har bir xususiyat
uchun alohida massiv (**SoA**, structure of arrays):

```cpp
struct Simulation {
    std::vector<Vec2> positions;
    std::vector<Vec2> velocities;
};
```

`std::vector<Vec2>` xotirada `x0 y0 x1 y1 ...` ko'rinishida zich yotadi, ya'ni `positions.data()` GPU kutayotgan baytlarning
aynan o'zi. Keyingi darslarda qo'shni qidirish tsikllari ko'p marta faqat o'rinlarni o'qiydi. SoA'da keraksiz ma'lumot keshni
to'ldirmaydi.

**Shader'ga tegmaymiz.** `in_position` 0-attribute, `in_color` 1-attribute bo'lib qoladi. Ular qaysi buferdan kelishi VAO
ichida yoziladi. Bu joyda 02-darsdagi qoidani eslang: `glVertexAttribPointer` chaqirilgan paytda `GL_ARRAY_BUFFER` ga qaysi bufer
bog'langan bo'lsa, attribute o'sha buferni **eslab qoladi**. `GL_ARRAY_BUFFER` bog'lanishining o'zi VAO'ga kirmaydi. Demak:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);    // 0-attribute: position_vbo

glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Color), nullptr);   // 1-attribute: color_vbo
```

Har bir buferda endi bitta narsa turadi, shuning uchun `offsetof` kerak emas, oxirgi argument (bufer boshidan siljish) `nullptr`.
Kadr ichidagi `glBindBuffer` chizishga ta'sir qilmaydi, u faqat keyingi `glBufferData` qaysi buferga yozishini aytadi.

## 10. Har kadrda yangilanadigan VBO

CPU va GPU bir vaqtda ishlaydi va GPU orqada keladi. `glDrawArraysInstanced` qaytganda chizish hali boshlanmagan bo'lishi mumkin,
buyruq navbatda turadi. Agar keyingi kadrda shu buferni ustidan yozsak, GPU hali eski ma'lumotni o'qiyotgan bo'lishi mumkin.
OpenGL natija to'g'ri bo'lishini kafolatlaydi, ya'ni drayver yo kutishi, yo ma'lumotni nusxalashi kerak.

Biz drayverga yordam beramiz: butun buferni qaytadan berish:

```cpp
glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
glBufferData(GL_ARRAY_BUFFER, sim.positions.size() * sizeof(Vec2), sim.positions.data(), GL_STREAM_DRAW);
```

`glBufferData` "bu buferning eski mazmuni endi kerak emas" degani. Drayver odatda yangi xotira ajratadi, GPU esa eski xotirani
o'qib bo'lgach, u bo'shatiladi. Hech kim hech kimni kutmaydi. Bunga **orphaning** deyiladi. Oshpaz eski retsept bilan ishlayotganda
qog'ozni qo'lidan tortib olib o'chirmaysiz, yangi varaqqa yozib stolga qo'yasiz.

Oxirgi argument **usage hint**, drayverga ishora: `GL_STATIC_DRAW` (bir marta yoziladi), `GL_DYNAMIC_DRAW` (tez-tez),
`GL_STREAM_DRAW` (har kadrda, ko'p ishlatilmaydi). Bu faqat ishora, buyruq emas. O'lchov (1 001 880 zarracha, vsync o'chiq,
RTX 3060, 7 ta o'lchovning o'rtachasi):

| Usul | FPS |
|---|---|
| `glBufferData` + `GL_STREAM_DRAW` | 126 |
| `glBufferData` + `GL_STATIC_DRAW` | 124 |
| bir marta ajratib, `glBufferSubData` | 116 |

Birinchi ikki qator farqi o'lchov shovqini ichida: bu kartada ishora natijaga ta'sir qilmadi. Boshqa drayverlar esa unga qarab
xotira turini tanlashi mumkin, shuning uchun to'g'risini yozamiz. Bu kadrlarda vaqtning asosiy qismi fizikada: bir qadam 2.17 ms, kadrda 3 qadam. Bizning 4536 zarrachada esa har kadrda
atigi 36 288 bayt yuboriladi va vsync o'chiq holda 7800 FPS atrofida chiqadi.

## 11. Spawner: zarrachalarni boshlang'ich joyiga qo'yish

Blok uchta narsa bilan beriladi: markazi, o'lchami (6.42 × 4.39) va **zichlik** (`density = 159`). Zichlik bu 1 × 1 o'lchamli kvadrat ichiga nechta zarracha
to'g'ri kelishi. Bizga esa boshqa sonlar kerak: gorizontal qatorda nechta zarracha (`nx`) va vertikal ustunda nechta (`ny`).

**Kvadrat ildiz.** 8-bo'limda ko'rdik: kvadrat bu sonni o'ziga ko'paytirish, `4 × 4 = 16`. Uning teskarisi **kvadrat ildiz**: "qaysi son o'ziga ko'paytirilsa 16 chiqadi?" Javob 4,
ya'ni `sqrt(16) = 4`.

1 × 1 kvadratga 159 ta zarrachani tekis termoqchimiz. Tekis bo'lishi uchun qatorlar soni ham, har qatordagi zarracha soni ham bir xil bo'lishi kerak, shu son `n` bo'lsin. Unda
`n × n = 159`, demak `n = sqrt(159) = 12.61`. Bu "1 birlik uzunlikka 12.61 ta zarracha" degani. Blokka qo'llaymiz:

```
eni:   6.42 × 12.61 = 80.95
bo'yi: 4.39 × 12.61 = 55.36
```

Zarracha soni butun bo'lishi kerak. **`std::ceil`** sonni yuqoriga yaxlitlaydi: `ceil(80.95) = 81`, `ceil(55.36) = 56`. Jami `81 × 56 = 4536`. 03-darsdagi sonlar shu yerdan olingan edi.

**Har bir zarrachaning joyi.** Sikl ichida `tx = i / (nx - 1)` zarrachaning qatordagi joyini 0 dan 1 gacha son bilan aytadi: `i = 0` da `0 / 80 = 0`, `i = 40` da `40 / 80 = 0.5`,
`i = 80` da `80 / 80 = 1`. Keyin `(tx - 0.5) × 6.42` uni markaz atrofiga joylaydi:

```
tx = 0:    (0 - 0.5) × 6.42  = -3.21     chap chet
tx = 0.5:  (0.5 - 0.5) × 6.42 = 0        o'rta
tx = 1:    (1 - 0.5) × 6.42  = 3.21      o'ng chet
```

Unga `spawn.centre.x` ni qo'shsak, blok kerakli joyga tushadi. `y` uchun ham xuddi shunday: markaz `0.66`, shuning uchun blok 03-darsdagidan biroz yuqorida boshlanadi.

**Jitter** (titrash) har bir zarrachani biroz tasodifiy siljitish. Nega kerak? Mukammal panjarada har bir zarrachaga atrofdan kelayotgan kuchlar har tomondan teng bo'lib, bir-birini
yo'qqa chiqaradi, suyuqlik esa sun'iy panjara shaklida turib qolishga moyil bo'ladi. Tabiatda bunday tartib bo'lmaydi. Kichik tasodif bu simmetriyani buzadi.

Jitter uchun kodda shu to'rtta qator bor:

```cpp
const float angle = random01(rng) * two_pi;
const float jitter = spawn.jitter * (random01(rng) - 0.5f);
...
position.x = spawn.centre.x + (tx - 0.5f) * spawn.size.x + std::cos(angle) * jitter;
position.y = spawn.centre.y + (ty - 0.5f) * spawn.size.y + std::sin(angle) * jitter;
```

**Tasodifiy son.** `random01(rng)` 0 bilan 1 orasidagi tasodifiy son beradi. Kompyuter sonlarni aslida formula bilan hosil qiladi va u boshlang'ich songa bog'liq. Bu boshlang'ich son **seed** deyiladi,
bizda `std::mt19937 rng(42)` dagi 42. Seed bir xil bo'lsa, har ishga tushishda bir xil sonlar chiqadi va xatoni qayta ko'rish oson bo'ladi. Faqat
`uniform_real_distribution` algoritmi kutubxonaga bog'liq, shuning uchun Windows va Mac'da jitter biroz farq qilishi mumkin. Bizga bu muhim emas.

**Tasodifiy yo'nalish: `cos` va `sin`.** Zarrachani qaysi tomonga siljitamiz? Istalgan tomonga. Radiusi 1 bo'lgan doirani tasavvur qiling. Markazdan biror burchak ostida strelka chizsangiz, uning uchi
doira ustiga tushadi. O'sha nuqtaning koordinatalari aynan `cos` va `sin`:

```
x = cos(burchak)
y = sin(burchak)
```

```
              (0, 1)
             .-''''-.
           /          \
  (-1, 0) |     ●──────| (1, 0)
           \          /
             '-....-'
              (0, -1)
```

Bizga `cos` va `sin` ning hozircha shu ma'nosi kerak: burchakni yo'nalishga aylantiradi.

| burchak (gradus) | burchak (radian) | `cos` | `sin` | yo'nalish |
|---|---|---|---|---|
| 0° | 0 | 1 | 0 | o'ngga |
| 45° | 0.7854 | 0.707 | 0.707 | o'ng-yuqoriga |
| 90° | 1.5708 | 0 | 1 | yuqoriga |
| 180° | 3.1416 | −1 | 0 | chapga |
| 270° | 4.7124 | 0 | −1 | pastga |

**Radian** burchakning kompyuter ishlatadigan o'lchovi. To'liq aylana 360° o'rniga 6.2832 ga teng (bu `pi` sonining ikki baravari, `pi` esa 3.1416). Shuning uchun `random01 × 6.2832` 0 dan to'liq aylanagacha
tasodifiy burchak beradi.

**Qancha siljitamiz.** `random01 - 0.5` soni −0.5 dan +0.5 gacha bo'ladi, 0.03 ga ko'paytirilsa −0.015 dan +0.015 gacha. Zarrachalar orasidagi masofa 0.08 atrofida, demak siljish bu masofaning beshdan biridan
kam. So'ng `cos(angle) * jitter` strelkaning `x` qismini, `sin(angle) * jitter` esa `y` qismini beradi. Ya'ni 1-bo'limdagi "vektorni songa ko'paytirish" amali.

**Rang.** `make_colors` boshlang'ich o'rindan `tx` ni qaytadan topadi: `(p.x - centre.x) / size.x + 0.5`. Bu yuqoridagi hisobning teskarisi: `x = -3.21` da `-3.21 / 6.42 + 0.5 = 0`, `x = 0` da `0.5`, `x = 3.21` da `1`.
Rang kanallari ham 0 dan 1 gacha bo'ladi: `0.2 + 0.8 × tx` chap chetda 0.2, o'ng chetda 1.0 beradi.

## 12. Klaviatura: pauza, qadam, qayta boshlash

`glfwGetKey` "tugma hozir bosilganmi" deb so'raydi. Pauza uchun bu noqulay: tugma bir necha kadr bosilib turadi va 165 Hz da holat
ko'p marta almashadi. Bizga **hodisa** kerak: "tugma bosildi". GLFW buni callback orqali beradi.

Callback oddiy C funksiya, u `Controls` ni o'z-o'zidan ko'rmaydi. GLFW buning uchun oynaga bitta ko'rsatkich bog'lashga ruxsat beradi:

```cpp
glfwSetWindowUserPointer(window, &controls);                                   // sikldan oldin
Controls* controls = static_cast<Controls*>(glfwGetWindowUserPointer(window)); // callback ichida
```

`action` uch xil bo'ladi: `GLFW_PRESS`, `GLFW_RELEASE`, `GLFW_REPEAT` (tugma bosib turilganda). Faqat `GLFW_PRESS` ni olamiz.

| Tugma | Vazifa |
|---|---|
| Space | pauza / davom |
| O'ng strelka | pauzada bitta kadr oldinga |
| R | boshlang'ich holat va pauza (asl Unity loyihasidagi kabi) |
| Esc | chiqish |

Callback faqat bayroqlarni o'rnatadi, simulyatsiyani asosiy sikl o'zgartiradi. Pauzada ham `last_time = now` ishlayveradi,
shuning uchun pauzadan keyingi `dt` ga pauza vaqti qo'shilmaydi.

## 13. Kodni yozamiz

### 13.1. `CMakeLists.txt`

`add_executable` ro'yxatiga yangi faylni qo'shing:

```cmake
add_executable(fluid_sim
    src/main.cpp
    src/gl.cpp
    src/shader.cpp
    src/sim2d.cpp
)
```

### 13.2. `src/sim2d.hpp`

```cpp
#pragma once

#include <vector>

struct Vec2 {
    float x, y;
};

inline Vec2 operator*(Vec2 v, float s) {
    return {v.x * s, v.y * s};
}

inline Vec2& operator+=(Vec2& a, Vec2 b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

struct SimSettings {
    float gravity = -12.0f;
    float collision_damping = 0.95f;
    Vec2 bounds_size = {17.1f, 9.3f};
    float max_frame_time = 1.0f / 60.0f;
    int iterations_per_frame = 3;
};

struct SpawnSettings {
    Vec2 centre = {0.0f, 0.66f};
    Vec2 size = {6.42f, 4.39f};
    float density = 159.0f;
    float jitter = 0.03f;
    Vec2 initial_velocity = {0.0f, 0.0f};
};

struct Simulation {
    std::vector<Vec2> positions;
    std::vector<Vec2> velocities;
};

Simulation spawn_particles(const SpawnSettings& spawn);
void step_simulation(Simulation& sim, const SimSettings& settings, float dt);
```

**`inline`** kerak, chunki funksiya tanasi header'da turibdi va header bir nechta `.cpp` ga kiradi. `inline` bo'lmasa, linker
bitta funksiyaning bir nechta nusxasini ko'rib, xato beradi.

**Operatorlar.** Hozircha faqat ishlatiladigan ikkitasi. `a += b` chapdagi obyektni o'zgartiradi, shuning uchun `a` havola
(`Vec2&`) bo'lib keladi. `Vec2` ikki `float`, uni nusxalash arzon, shuning uchun qolgan joylarda qiymat bilan uzatiladi.

**`SimSettings` va `SpawnSettings`**: barcha sonlar bir joyda. Qiymatlar asl loyihaning "Test A" sahnasidan olingan.

### 13.3. `src/sim2d.cpp`

```cpp
#include "sim2d.hpp"

#include <cmath>
#include <cstddef>
#include <random>

Simulation spawn_particles(const SpawnSettings& spawn) {
    const float per_unit = std::sqrt(spawn.density);
    const int nx = static_cast<int>(std::ceil(spawn.size.x * per_unit));
    const int ny = static_cast<int>(std::ceil(spawn.size.y * per_unit));

    Simulation sim;
    sim.positions.reserve(nx * ny);
    sim.velocities.reserve(nx * ny);

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> random01(0.0f, 1.0f);
    const float two_pi = 6.2831853f;

    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            const float tx = i / (nx - 1.0f);
            const float ty = j / (ny - 1.0f);

            const float angle = random01(rng) * two_pi;
            const float jitter = spawn.jitter * (random01(rng) - 0.5f);

            Vec2 position;
            position.x = spawn.centre.x + (tx - 0.5f) * spawn.size.x + std::cos(angle) * jitter;
            position.y = spawn.centre.y + (ty - 0.5f) * spawn.size.y + std::sin(angle) * jitter;

            sim.positions.push_back(position);
            sim.velocities.push_back(spawn.initial_velocity);
        }
    }
    return sim;
}

static void apply_gravity(Simulation& sim, const SimSettings& settings, float dt) {
    const Vec2 acceleration = {0.0f, settings.gravity};

    for (Vec2& velocity : sim.velocities) {
        velocity += acceleration * dt;
    }
}

static void resolve_collisions(Vec2& position, Vec2& velocity, const SimSettings& settings) {
    const float half_width = settings.bounds_size.x * 0.5f;
    const float half_height = settings.bounds_size.y * 0.5f;

    if (std::fabs(position.x) >= half_width) {
        position.x = std::copysign(half_width, position.x);
        velocity.x *= -settings.collision_damping;
    }
    if (std::fabs(position.y) >= half_height) {
        position.y = std::copysign(half_height, position.y);
        velocity.y *= -settings.collision_damping;
    }
}

static void update_positions(Simulation& sim, const SimSettings& settings, float dt) {
    for (std::size_t i = 0; i < sim.positions.size(); i++) {
        sim.positions[i] += sim.velocities[i] * dt;
        resolve_collisions(sim.positions[i], sim.velocities[i], settings);
    }
}

void step_simulation(Simulation& sim, const SimSettings& settings, float dt) {
    apply_gravity(sim, settings, dt);
    update_positions(sim, settings, dt);
}
```

**`static`** funksiyalar faqat shu `.cpp` ichida ko'rinadi. Tashqariga faqat header'dagi ikkitasi chiqadi.

**`for (Vec2& velocity : ...)`** dagi `&` muhim. Usiz sikl har elementning nusxasini oladi, nusxa o'zgaradi va massiv o'zgarishsiz
qoladi. Compiler bu haqda ogohlantirmaydi.

**`step_simulation` tartibi** semi-implicit Euler'ning o'zi: avval hamma tezlik, keyin hamma o'rin. 05 va 06-darslarda bu ikki
chaqiruv orasiga zichlik va bosim qo'shiladi.

### 13.4. `src/main.cpp` (to'liq almashtiring)

```cpp
#include "gl.hpp"
#include "shader.hpp"
#include "sim2d.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <vector>

struct Color {
    float r, g, b;
};

struct Controls {
    bool paused = false;
    bool step_once = false;
    bool reset = false;
};

static void on_glfw_error(int code, const char* description) {
    std::fprintf(stderr, "GLFW xatosi %d: %s\n", code, description);
}

static void on_key(GLFWwindow* window, int key, int, int action, int) {
    if (action != GLFW_PRESS) {
        return;
    }

    Controls* controls = static_cast<Controls*>(glfwGetWindowUserPointer(window));
    switch (key) {
    case GLFW_KEY_SPACE: controls->paused = !controls->paused; break;
    case GLFW_KEY_RIGHT: controls->step_once = true; break;
    case GLFW_KEY_R: controls->reset = true; break;
    case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    default: break;
    }
}

static const char* gl_string(GLenum name) {
    return reinterpret_cast<const char*>(glGetString(name));
}

static std::vector<Color> make_colors(const std::vector<Vec2>& positions, const SpawnSettings& spawn) {
    std::vector<Color> colors;
    colors.reserve(positions.size());

    for (const Vec2& p : positions) {
        const float tx = (p.x - spawn.centre.x) / spawn.size.x + 0.5f;
        const float ty = (p.y - spawn.centre.y) / spawn.size.y + 0.5f;
        colors.push_back({0.2f + 0.8f * tx, 0.2f + 0.8f * ty, 1.0f - 0.7f * tx});
    }
    return colors;
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

    Controls controls;
    glfwSetWindowUserPointer(window, &controls);
    glfwSetKeyCallback(window, on_key);

    GLuint program = create_program("particle2d.vert", "particle2d.frag");
    if (!program) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    const GLint loc_world_to_clip = uniform_location(program, "u_world_to_clip");
    const GLint loc_diameter = uniform_location(program, "u_diameter");

    const SimSettings settings;
    const SpawnSettings spawn;
    const Simulation initial = spawn_particles(spawn);
    Simulation sim = initial;

    const std::vector<Color> colors = make_colors(initial.positions, spawn);
    const int particle_count = static_cast<int>(sim.positions.size());

    GLuint vao = 0;
    GLuint position_vbo = 0;
    GLuint color_vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &position_vbo);
    glGenBuffers(1, &color_vbo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2), nullptr);
    glVertexAttribDivisor(0, 1);

    glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(Color), colors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Color), nullptr);
    glVertexAttribDivisor(1, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl_check("sozlash");

    double last_time = glfwGetTime();
    double fps_time = last_time;
    int fps_frames = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        const double now = glfwGetTime();
        const float frame_time = static_cast<float>(now - last_time);
        last_time = now;

        if (controls.reset) {
            sim = initial;
            controls.reset = false;
            controls.paused = true;
        }

        if (!controls.paused || controls.step_once) {
            const float dt = std::min(frame_time, settings.max_frame_time) / settings.iterations_per_frame;
            for (int i = 0; i < settings.iterations_per_frame; i++) {
                step_simulation(sim, settings, dt);
            }
        }
        controls.step_once = false;

        int fb_width = 0;
        int fb_height = 0;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        glViewport(0, 0, fb_width, fb_height);

        glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        const float aspect = static_cast<float>(std::max(fb_width, 1)) / std::max(fb_height, 1);
        float half_height = 5.0f;
        float half_width = half_height * aspect;

        const float min_half_width = settings.bounds_size.x * 0.5f * 1.02f;
        if (half_width < min_half_width) {
            half_width = min_half_width;
            half_height = half_width / aspect;
        }

        glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
        glBufferData(GL_ARRAY_BUFFER, sim.positions.size() * sizeof(Vec2), sim.positions.data(), GL_STREAM_DRAW);

        glUseProgram(program);
        glUniform2f(loc_world_to_clip, 1.0f / half_width, 1.0f / half_height);
        glUniform1f(loc_diameter, 0.08f);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, particle_count);
        gl_check("kadr");

        glfwSwapBuffers(window);

        fps_frames++;
        if (now - fps_time >= 0.5) {
            int win_width = 0;
            int win_height = 0;
            glfwGetWindowSize(window, &win_width, &win_height);

            char title[128];
            std::snprintf(title, sizeof(title), "Fluid Sim | %.0f FPS | %d zarracha | oyna %dx%d | framebuffer %dx%d%s",
                          fps_frames / (now - fps_time), particle_count, win_width, win_height, fb_width, fb_height,
                          controls.paused ? " | pauza" : "");
            glfwSetWindowTitle(window, title);

            fps_time = now;
            fps_frames = 0;
        }
    }

    glDeleteBuffers(1, &position_vbo);
    glDeleteBuffers(1, &color_vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(program);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
```

03-darsdan farqlari:

**`on_key`** imzosini GLFW belgilaydi. Bizga `scancode` va `mods` kerak emas, parametr nomini yozmasak, `-Wextra` "ishlatilmagan
parametr" deb ogohlantirmaydi.

**`Controls controls`** `main` ichida yashaydi va oyna yopilguncha turadi, shuning uchun unga ko'rsatkich xavfsiz.

**`make_colors`** har zarrachaga boshlang'ich o'rniga qarab rang beradi. Rang zarrachaga bir marta bog'lanadi, shuning uchun
yiqilish paytida qaysi zarracha qayerdan kelganini ko'rasiz. 05-darsda u zichlik rangi bilan almashadi.

**`const Simulation initial`** boshlang'ich holatning nusxasi. `sim = initial` bitta qatorda ikkala vektorni ko'chiradi, R tugmasi
shu bilan ishlaydi.

**`position_vbo` ga sikldan oldin ma'lumot yozilmaydi.** `glVertexAttribPointer` xotira mavjudligini talab qilmaydi, birinchi
`glBufferData` birinchi kadrda chizishdan oldin keladi.

**`min_half_width`**. Devorlar paydo bo'ldi va ular ekranda ko'rinishi kerak. `aspect` oynaning eni bo'yidan necha baravar katta
ekanini bildiradi (03-dars). Masalan, 640 x 720 oynada `aspect = 640 / 720 = 0.889` va `half_width = 5 * 0.889 = 4.44`, ya'ni ekranning
chetigacha 4.44 birlik ko'rinadi, devor esa 8.55 da turibdi va ko'rinmay qoladi. Shunday holatda `half_width` ni qutiga moslab
kattalashtiramiz: `8.55 * 1.02 = 8.72`. 1.02 bilan ko'paytirish 2% ortiqcha joy qoldiradi. Doiralar cho'zilmasligi uchun `half_height` ham
xuddi shu nisbatda kattalashadi.

### 13.5. Yig'ish va ishga tushirish

```
cmake --build build
.\build\fluid_sim.exe      # Windows (PowerShell)
./build/fluid_sim          # macOS
```

`CMakeLists.txt` o'zgargani uchun CMake o'zi qayta sozlanadi.

## 14. Ishlaganini qanday tekshirasiz

**Windows.** Bu natijani o'zim ko'rdim (1280×720, RTX 3060):

- Blok 03-darsdagidan biroz yuqorida boshlanadi (markazi `y = 0.66`) va panjara endi jitter tufayli biroz notekis.
- Taxminan 0.7 soniyada pastki qator polga urilib, yuqoriga qaytadi. Yuqoridan tushayotgan qatorlar ular orasidan o'tib ketadi.
- 40 soniyaga kelib hamma zarracha oynaning pastida bitta chiziqqa yig'iladi (framebuffer'ning yuqorisidan 695 piksel). Chiziq
  moviy-sariq, chunki eng oxirgi chiziladigan yuqori qatorlar ustida qoladi.
- Space: sarlavhada `pauza` chiqadi, rasm qotadi (ikki skrinshot piksel-piksel bir xil chiqdi). O'ng strelka: bitta kadr oldinga.
  R: boshlang'ich panjara va pauza, necha marta bossangiz ham bir xil rasm.
- Oynani 640×720 qilsangiz, ko'rinish kengayadi: blok 238 piksel enida va devorlar ekranda qoladi.
- Sarlavhada 165 FPS va `4536 zarracha`, terminalda `OpenGL xatosi` yo'q.

**macOS.** Buni o'zim ishga tushira olmadim, kutilayotgan natija: xuddi shu harakat. Sarlavhadagi FPS monitorga bog'liq (60 yoki
120), lekin vaqtlar o'sha-o'sha: blok taxminan 0.7 soniyada polga uriladi va 40 soniyada chiziqqa yig'iladi. Retina ekranda
framebuffer oynadan ikki baravar katta bo'ladi.

**Nima buzilsa nima bo'ladi.** Hammasini ishga tushirib ko'rdim. OpenGL qatorlari RTX 3060 dagi natija, boshqa drayverda
boshqacha bo'lishi mumkin:

| O'zgarish | Natija | Xabar bormi |
|---|---|---|
| kadr ichidagi `glBindBuffer(..., position_vbo)` yo'q | o'rinlar `color_vbo` ga yoziladi, `position_vbo` bo'sh qoladi, dastur 2–4 soniyada yiqiladi (`0xC0000005`) | yo'q, `gl_check` ham jim |
| kadr ichidagi `glBufferData` yo'q | xuddi shunday: xotirasi yo'q buferdan o'qish, yiqilish | yo'q |
| `glBufferData` faqat sikldan oldin | zarrachalar joyida qotadi, simulyatsiya esa orqada ishlayveradi | yo'q |
| `glVertexAttribPointer(1, ...)` `color_vbo` bog'lanishidan oldin | ranglar o'rin baytlaridan o'qiladi: yashil-binafsha naqsh, uch bo'lakka bo'lingan | yo'q |
| `glfwSetWindowUserPointer` yo'q | birinchi tugmada yiqiladi (`0xC0000005`, null ko'rsatkich) | yo'q |
| `action != GLFW_PRESS` tekshiruvi yo'q | Space pauza qilmaydi: bosish ham, qo'yib yuborish ham holatni almashtiradi | yo'q |
| `dt` iteratsiyalarga bo'linmagan | hamma narsa 3 baravar tez (polga 0.242 s da) | yo'q |
| `dt = 1/180` qotirilgan | 165 Hz da 2.75 baravar tez, 60 Hz da to'g'ri | yo'q |
| o'rin qaytarilmagan | hamma zarracha pol ostida qoladi | yo'q |
| tezlik teskari qilinmagan | rasm to'g'ri, lekin tezlik 120 gacha o'sadi | yo'q |

Birinchi qator yaxshi saboq: OpenGL buferga kim yozayotganini tekshirmaydi, `gl_check` jim, dastur esa birozdan keyin yiqiladi.
Xotirasi yo'q buferdan o'qishni OpenGL xato deb qaytarmaydi, natijasi aniqlanmagan (undefined behaviour).

## 15. Git va Mac

Windows'da:

```
git add .
git commit -m "Lesson 04: motion, timestep, wall collisions, streaming VBO"
git push
```

Mac'da:

```
git pull
cmake --build build
./build/fluid_sim
```

## 16. Ixtiyoriy tajribalar

1. `sim2d.hpp` da `initial_velocity = {8.0f, 3.0f}` qiling: zarrachalar yon devorlarga ham uriladi. Men 30 soniya yurgizdim, 0-zarracha
   10 marta urildi va tezligi 8 dan 4.79 ga tushdi (8 ni 10 marta 0.95 ga ko'paytirsangiz, shu son chiqadi), qutidan bironta zarracha chiqmadi.
2. `collision_damping = 0.5f` qiling va 8-bo'limdagi jadval bilan solishtiring.
3. `density = 35500.0f` va `glfwSwapInterval(0)`: bir million zarracha. Windows'da 126 FPS chiqdi, Mac'da qancha chiqishini yozing.

## 17. Lug'at

| So'z | Ma'nosi |
|---|---|
| koordinata | nuqta markazdan qayerdaligini aytuvchi sonlar `(x, y)` |
| vektor | birga yuradigan ikki son, strelka deb tasavvur qilinadi |
| o'rin (position) | zarracha qayerda |
| tezlik (velocity) | bir soniyada o'rin qanchaga o'zgaradi, yo'nalishi bilan |
| tezlanish (acceleration) | bir soniyada tezlik qanchaga o'zgaradi |
| kuch (force) | tezlikni o'zgartiruvchi itarish yoki tortish |
| massa (mass) | narsani tezlashtirish qanchalik qiyinligi |
| gravitatsiya | Yerning hamma narsani pastga tortishi, bizda tezlanish −12 |
| holat (state) | saqlanadigan ma'lumot: o'rin va tezlik |
| `dt`, timestep | bitta hisob qadamining davomiyligi, soniyada |
| Euler integratsiyasi | tezlik va o'rinni kichik qadamlar bilan yangilash |
| semi-implicit Euler | avval tezlik, keyin o'rin tartibi |
| tebranish | markaz atrofida borib-kelish |
| energiya | harakatda yoki joylashuvda yig'ilgan zaxira, yo'qdan paydo bo'lmaydi |
| barqarorlik (stability) | `dt` yetarlicha kichik bo'lganda hisobning portlamasligi |
| damping | urilishda tezlikning qancha qismi qolishi |
| kvadrat | sonni o'ziga ko'paytirish |
| `sqrt` | kvadrat ildiz: qaysi son o'ziga ko'paytirilsa shu son chiqadi |
| `ceil` | yuqoriga yaxlitlash |
| `fabs` | sonning ishorasini olib tashlash |
| `copysign(a, b)` | `a` ga `b` ning ishorasini berish |
| `cos`, `sin` | burchakni radiusi 1 doira ustidagi nuqtaga aylantirish |
| radian | burchak o'lchovi, to'liq aylana 6.2832 |
| seed | tasodifiy sonlar ketma-ketligining boshlang'ich soni |
| jitter | zarrachani biroz tasodifiy siljitish |

## Keyingi dars

05-darsda SPH'ning asosiy g'oyasiga o'tamiz: **smoothing kernel**, uning normalizatsiyasi va har bir zarrachadagi **zichlik**.
Zarrachalar zichlikka qarab bo'yaladi, 06-darsda esa zichlik bosimga aylanib, birinchi haqiqiy suyuqlik paydo bo'ladi.

Shu dars tugagach yozing. Xohlasangiz, kodingizni ko'rib chiqaman.
