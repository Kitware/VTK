// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2011 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkMath.h"

#include "vtkBoxMuellerRandomSequence.h"
#include "vtkDataArray.h"
#include "vtkDebugLeaks.h"
#include "vtkMinimalStandardRandomSequence.h"
#include "vtkObjectFactory.h"
#include "vtkTypeTraits.h"

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkMath);

namespace
{

// clang-format off
// The first 55 rows of Pascal's triangle.
// Double-precision floating-point numbers exactly
// represent these and no more (only 52 bits of mantissa
// exist in IEEE 754 double-precision numbers, and the
// values in the final row consume over 51 bits).
//
// This is used by vtkMath::DoubleBinomial() to perform
// table-based lookup of binomial coefficients.
std::array<double, 1596> vtkPascalTriangle{
  1,
  1, 1,
  1, 2, 1,
  1, 3, 3, 1,
  1, 4, 6, 4, 1,
  1, 5, 10, 10, 5, 1,
  1, 6, 15, 20, 15, 6, 1,
  1, 7, 21, 35, 35, 21, 7, 1,
  1, 8, 28, 56, 70, 56, 28, 8, 1,
  1, 9, 36, 84, 126, 126, 84, 36, 9, 1,
  1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1,
  1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1,
  1, 12, 66, 220, 495, 792, 924, 792, 495, 220, 66, 12, 1,
  1, 13, 78, 286, 715, 1287, 1716, 1716, 1287, 715, 286, 78, 13, 1,
  1, 14, 91, 364, 1001, 2002, 3003, 3432, 3003, 2002, 1001, 364, 91, 14, 1,
  1, 15, 105, 455, 1365, 3003, 5005, 6435, 6435, 5005, 3003, 1365, 455, 105, 15, 1,
  1, 16, 120, 560, 1820, 4368, 8008, 11440, 12870, 11440, 8008, 4368, 1820, 560, 120, 16, 1,
  1, 17, 136, 680, 2380, 6188, 12376, 19448, 24310, 24310, 19448, 12376, 6188, 2380, 680, 136, 17, 1,
  1, 18, 153, 816, 3060, 8568, 18564, 31824, 43758, 48620, 43758, 31824, 18564, 8568, 3060, 816, 153, 18, 1,
  1, 19, 171, 969, 3876, 11628, 27132, 50388, 75582, 92378, 92378, 75582, 50388, 27132, 11628, 3876, 969, 171, 19, 1,
  1, 20, 190, 1140, 4845, 15504, 38760, 77520, 125970, 167960, 184756, 167960, 125970, 77520, 38760, 15504, 4845, 1140, 190, 20, 1,
  1, 21, 210, 1330, 5985, 20349, 54264, 116280, 203490, 293930, 352716, 352716, 293930, 203490, 116280, 54264, 20349, 5985, 1330, 210, 21, 1,
  1, 22, 231, 1540, 7315, 26334, 74613, 170544, 319770, 497420, 646646, 705432, 646646, 497420, 319770, 170544, 74613, 26334, 7315, 1540, 231, 22, 1,
  1, 23, 253, 1771, 8855, 33649, 100947, 245157, 490314, 817190, 1144066, 1352078, 1352078, 1144066, 817190, 490314, 245157, 100947, 33649, 8855, 1771, 253, 23, 1,
  1, 24, 276, 2024, 10626, 42504, 134596, 346104, 735471, 1307504, 1961256, 2496144, 2704156, 2496144, 1961256, 1307504, 735471, 346104, 134596, 42504, 10626, 2024, 276, 24, 1,
  1, 25, 300, 2300, 12650, 53130, 177100, 480700, 1081575, 2042975, 3268760, 4457400, 5200300, 5200300, 4457400, 3268760, 2042975, 1081575, 480700, 177100, 53130, 12650, 2300, 300, 25, 1,
  1, 26, 325, 2600, 14950, 65780, 230230, 657800, 1562275, 3124550, 5311735, 7726160, 9657700, 10400600, 9657700, 7726160, 5311735, 3124550, 1562275, 657800, 230230, 65780, 14950, 2600, 325, 26, 1,
  1, 27, 351, 2925, 17550, 80730, 296010, 888030, 2220075, 4686825, 8436285, 13037895, 17383860, 20058300, 20058300, 17383860, 13037895, 8436285, 4686825, 2220075, 888030, 296010, 80730, 17550, 2925, 351, 27, 1,
  1, 28, 378, 3276, 20475, 98280, 376740, 1184040, 3108105, 6906900, 13123110, 21474180, 30421755, 37442160, 40116600, 37442160, 30421755, 21474180, 13123110, 6906900, 3108105, 1184040, 376740, 98280, 20475, 3276, 378, 28, 1,
  1, 29, 406, 3654, 23751, 118755, 475020, 1560780, 4292145, 10015005, 20030010, 34597290, 51895935, 67863915, 77558760, 77558760, 67863915, 51895935, 34597290, 20030010, 10015005, 4292145, 1560780, 475020, 118755, 23751, 3654, 406, 29, 1,
  1, 30, 435, 4060, 27405, 142506, 593775, 2035800, 5852925, 14307150, 30045015, 54627300, 86493225, 119759850, 145422675, 155117520, 145422675, 119759850, 86493225, 54627300, 30045015, 14307150, 5852925, 2035800, 593775, 142506, 27405, 4060, 435, 30, 1,
  1, 31, 465, 4495, 31465, 169911, 736281, 2629575, 7888725, 20160075, 44352165, 84672315, 141120525, 206253075, 265182525, 300540195, 300540195, 265182525, 206253075, 141120525, 84672315, 44352165, 20160075, 7888725, 2629575, 736281, 169911, 31465, 4495, 465, 31, 1,
  1, 32, 496, 4960, 35960, 201376, 906192, 3365856, 10518300, 28048800, 64512240, 129024480, 225792840, 347373600, 471435600, 565722720, 601080390, 565722720, 471435600, 347373600, 225792840, 129024480, 64512240, 28048800, 10518300, 3365856, 906192, 201376, 35960, 4960, 496, 32, 1,
  1, 33, 528, 5456, 40920, 237336, 1107568, 4272048, 13884156, 38567100, 92561040, 193536720, 354817320, 573166440, 818809200, 1037158320, 1166803110, 1166803110, 1037158320, 818809200, 573166440, 354817320, 193536720, 92561040, 38567100, 13884156, 4272048, 1107568, 237336, 40920, 5456, 528, 33, 1,
  1, 34, 561, 5984, 46376, 278256, 1344904, 5379616, 18156204, 52451256, 131128140, 286097760, 548354040, 927983760, 1391975640, 1855967520, 2203961430, 2333606220, 2203961430, 1855967520, 1391975640, 927983760, 548354040, 286097760, 131128140, 52451256, 18156204, 5379616, 1344904, 278256, 46376, 5984, 561, 34, 1,
  1, 35, 595, 6545, 52360, 324632, 1623160, 6724520, 23535820, 70607460, 183579396, 417225900, 834451800, 1476337800, 2319959400, 3247943160, 4059928950, 4537567650, 4537567650, 4059928950, 3247943160, 2319959400, 1476337800, 834451800, 417225900, 183579396, 70607460, 23535820, 6724520, 1623160, 324632, 52360, 6545, 595, 35, 1,
  1, 36, 630, 7140, 58905, 376992, 1947792, 8347680, 30260340, 94143280, 254186856, 600805296, 1251677700, 2310789600, 3796297200, 5567902560, 7307872110, 8597496600, 9075135300, 8597496600, 7307872110, 5567902560, 3796297200, 2310789600, 1251677700, 600805296, 254186856, 94143280, 30260340, 8347680, 1947792, 376992, 58905, 7140, 630, 36, 1,
  1, 37, 666, 7770, 66045, 435897, 2324784, 10295472, 38608020, 124403620, 348330136, 854992152, 1852482996, 3562467300, 6107086800, 9364199760, 12875774670, 15905368710, 17672631900, 17672631900, 15905368710, 12875774670, 9364199760, 6107086800, 3562467300, 1852482996, 854992152, 348330136, 124403620, 38608020, 10295472, 2324784, 435897, 66045, 7770, 666, 37, 1,
  1, 38, 703, 8436, 73815, 501942, 2760681, 12620256, 48903492, 163011640, 472733756, 1203322288, 2707475148, 5414950296, 9669554100, 15471286560, 22239974430, 28781143380, 33578000610, 35345263800, 33578000610, 28781143380, 22239974430, 15471286560, 9669554100, 5414950296, 2707475148, 1203322288, 472733756, 163011640, 48903492, 12620256, 2760681, 501942, 73815, 8436, 703, 38, 1,
  1, 39, 741, 9139, 82251, 575757, 3262623, 15380937, 61523748, 211915132, 635745396, 1676056044, 3910797436, 8122425444, 15084504396, 25140840660, 37711260990, 51021117810, 62359143990, 68923264410, 68923264410, 62359143990, 51021117810, 37711260990, 25140840660, 15084504396, 8122425444, 3910797436, 1676056044, 635745396, 211915132, 61523748, 15380937, 3262623, 575757, 82251, 9139, 741, 39, 1,
  1, 40, 780, 9880, 91390, 658008, 3838380, 18643560, 76904685, 273438880, 847660528, 2311801440, 5586853480, 12033222880, 23206929840, 40225345056, 62852101650, 88732378800, 113380261800, 131282408400, 137846528820, 131282408400, 113380261800, 88732378800, 62852101650, 40225345056, 23206929840, 12033222880, 5586853480, 2311801440, 847660528, 273438880, 76904685, 18643560, 3838380, 658008, 91390, 9880, 780, 40, 1,
  1, 41, 820, 10660, 101270, 749398, 4496388, 22481940, 95548245, 350343565, 1121099408, 3159461968, 7898654920, 17620076360, 35240152720, 63432274896, 103077446706, 151584480450, 202112640600, 244662670200, 269128937220, 269128937220, 244662670200, 202112640600, 151584480450, 103077446706, 63432274896, 35240152720, 17620076360, 7898654920, 3159461968, 1121099408, 350343565, 95548245, 22481940, 4496388, 749398, 101270, 10660, 820, 41, 1,
  1, 42, 861, 11480, 111930, 850668, 5245786, 26978328, 118030185, 445891810, 1471442973, 4280561376, 11058116888, 25518731280, 52860229080, 98672427616, 166509721602, 254661927156, 353697121050, 446775310800, 513791607420, 538257874440, 513791607420, 446775310800, 353697121050, 254661927156, 166509721602, 98672427616, 52860229080, 25518731280, 11058116888, 4280561376, 1471442973, 445891810, 118030185, 26978328, 5245786, 850668, 111930, 11480, 861, 42, 1,
  1, 43, 903, 12341, 123410, 962598, 6096454, 32224114, 145008513, 563921995, 1917334783, 5752004349, 15338678264, 36576848168, 78378960360, 151532656696, 265182149218, 421171648758, 608359048206, 800472431850, 960566918220, 1052049481860, 1052049481860, 960566918220, 800472431850, 608359048206, 421171648758, 265182149218, 151532656696, 78378960360, 36576848168, 15338678264, 5752004349, 1917334783, 563921995, 145008513, 32224114, 6096454, 962598, 123410, 12341, 903, 43, 1,
  1, 44, 946, 13244, 135751, 1086008, 7059052, 38320568, 177232627, 708930508, 2481256778, 7669339132, 21090682613, 51915526432, 114955808528, 229911617056, 416714805914, 686353797976, 1029530696964, 1408831480056, 1761039350070, 2012616400080, 2104098963720, 2012616400080, 1761039350070, 1408831480056, 1029530696964, 686353797976, 416714805914, 229911617056, 114955808528, 51915526432, 21090682613, 7669339132, 2481256778, 708930508, 177232627, 38320568, 7059052, 1086008, 135751, 13244, 946, 44, 1,
  1, 45, 990, 14190, 148995, 1221759, 8145060, 45379620, 215553195, 886163135, 3190187286, 10150595910, 28760021745, 73006209045, 166871334960, 344867425584, 646626422970, 1103068603890, 1715884494940, 2438362177020, 3169870830126, 3773655750150, 4116715363800, 4116715363800, 3773655750150, 3169870830126, 2438362177020, 1715884494940, 1103068603890, 646626422970, 344867425584, 166871334960, 73006209045, 28760021745, 10150595910, 3190187286, 886163135, 215553195, 45379620, 8145060, 1221759, 148995, 14190, 990, 45, 1,
  1, 46, 1035, 15180, 163185, 1370754, 9366819, 53524680, 260932815, 1101716330, 4076350421, 13340783196, 38910617655, 101766230790, 239877544005, 511738760544, 991493848554, 1749695026860, 2818953098830, 4154246671960, 5608233007146, 6943526580276, 7890371113950, 8233430727600, 7890371113950, 6943526580276, 5608233007146, 4154246671960, 2818953098830, 1749695026860, 991493848554, 511738760544, 239877544005, 101766230790, 38910617655, 13340783196, 4076350421, 1101716330, 260932815, 53524680, 9366819, 1370754, 163185, 15180, 1035, 46, 1,
  1, 47, 1081, 16215, 178365, 1533939, 10737573, 62891499, 314457495, 1362649145, 5178066751, 17417133617, 52251400851, 140676848445, 341643774795, 751616304549, 1503232609098, 2741188875414, 4568648125690, 6973199770790, 9762479679106, 12551759587422, 14833897694226, 16123801841550, 16123801841550, 14833897694226, 12551759587422, 9762479679106, 6973199770790, 4568648125690, 2741188875414, 1503232609098, 751616304549, 341643774795, 140676848445, 52251400851, 17417133617, 5178066751, 1362649145, 314457495, 62891499, 10737573, 1533939, 178365, 16215, 1081, 47, 1,
  1, 48, 1128, 17296, 194580, 1712304, 12271512, 73629072, 377348994, 1677106640, 6540715896, 22595200368, 69668534468, 192928249296, 482320623240, 1093260079344, 2254848913647, 4244421484512, 7309837001104, 11541847896480, 16735679449896, 22314239266528, 27385657281648, 30957699535776, 32247603683100, 30957699535776, 27385657281648, 22314239266528, 16735679449896, 11541847896480, 7309837001104, 4244421484512, 2254848913647, 1093260079344, 482320623240, 192928249296, 69668534468, 22595200368, 6540715896, 1677106640, 377348994, 73629072, 12271512, 1712304, 194580, 17296, 1128, 48, 1,
  1, 49, 1176, 18424, 211876, 1906884, 13983816, 85900584, 450978066, 2054455634, 8217822536, 29135916264, 92263734836, 262596783764, 675248872536, 1575580702584, 3348108992991, 6499270398159, 11554258485616, 18851684897584, 28277527346376, 39049918716424, 49699896548176, 58343356817424, 63205303218876, 63205303218876, 58343356817424, 49699896548176, 39049918716424, 28277527346376, 18851684897584, 11554258485616, 6499270398159, 3348108992991, 1575580702584, 675248872536, 262596783764, 92263734836, 29135916264, 8217822536, 2054455634, 450978066, 85900584, 13983816, 1906884, 211876, 18424, 1176, 49, 1,
  1, 50, 1225, 19600, 230300, 2118760, 15890700, 99884400, 536878650, 2505433700, 10272278170, 37353738800, 121399651100, 354860518600, 937845656300, 2250829575120, 4923689695575, 9847379391150, 18053528883775, 30405943383200, 47129212243960, 67327446062800, 88749815264600, 108043253365600, 121548660036300, 126410606437752, 121548660036300, 108043253365600, 88749815264600, 67327446062800, 47129212243960, 30405943383200, 18053528883775, 9847379391150, 4923689695575, 2250829575120, 937845656300, 354860518600, 121399651100, 37353738800, 10272278170, 2505433700, 536878650, 99884400, 15890700, 2118760, 230300, 19600, 1225, 50, 1,
  1, 51, 1275, 20825, 249900, 2349060, 18009460, 115775100, 636763050, 3042312350, 12777711870, 47626016970, 158753389900, 476260169700, 1292706174900, 3188675231420, 7174519270695, 14771069086725, 27900908274925, 48459472266975, 77535155627160, 114456658306760, 156077261327400, 196793068630200, 229591913401900, 247959266474052, 247959266474052, 229591913401900, 196793068630200, 156077261327400, 114456658306760, 77535155627160, 48459472266975, 27900908274925, 14771069086725, 7174519270695, 3188675231420, 1292706174900, 476260169700, 158753389900, 47626016970, 12777711870, 3042312350, 636763050, 115775100, 18009460, 2349060, 249900, 20825, 1275, 51, 1,
  1, 52, 1326, 22100, 270725, 2598960, 20358520, 133784560, 752538150, 3679075400, 15820024220, 60403728840, 206379406870, 635013559600, 1768966344600, 4481381406320, 10363194502115, 21945588357420, 42671977361650, 76360380541900, 125994627894135, 191991813933920, 270533919634160, 352870329957600, 426384982032100, 477551179875952, 495918532948104, 477551179875952, 426384982032100, 352870329957600, 270533919634160, 191991813933920, 125994627894135, 76360380541900, 42671977361650, 21945588357420, 10363194502115, 4481381406320, 1768966344600, 635013559600, 206379406870, 60403728840, 15820024220, 3679075400, 752538150, 133784560, 20358520, 2598960, 270725, 22100, 1326, 52, 1,
  1, 53, 1378, 23426, 292825, 2869685, 22957480, 154143080, 886322710, 4431613550, 19499099620, 76223753060, 266783135710, 841392966470, 2403979904200, 6250347750920, 14844575908435, 32308782859535, 64617565719070, 119032357903550, 202355008436035, 317986441828055, 462525733568080, 623404249591760, 779255311989700, 903936161908052, 973469712824056, 973469712824056, 903936161908052, 779255311989700, 623404249591760, 462525733568080, 317986441828055, 202355008436035, 119032357903550, 64617565719070, 32308782859535, 14844575908435, 6250347750920, 2403979904200, 841392966470, 266783135710, 76223753060, 19499099620, 4431613550, 886322710, 154143080, 22957480, 2869685, 292825, 23426, 1378, 53, 1,
  1, 54, 1431, 24804, 316251, 3162510, 25827165, 177100560, 1040465790, 5317936260, 23930713170, 95722852680, 343006888770, 1108176102180, 3245372870670, 8654327655120, 21094923659355, 47153358767970, 96926348578605, 183649923622620, 321387366339585, 520341450264090, 780512175396135, 1085929983159840, 1402659561581460, 1683191473897752, 1877405874732108, 1946939425648112, 1877405874732108, 1683191473897752, 1402659561581460, 1085929983159840, 780512175396135, 520341450264090, 321387366339585, 183649923622620, 96926348578605, 47153358767970, 21094923659355, 8654327655120, 3245372870670, 1108176102180, 343006888770, 95722852680, 23930713170, 5317936260, 1040465790, 177100560, 25827165, 3162510, 316251, 24804, 1431, 54, 1,
  1, 55, 1485, 26235, 341055, 3478761, 28989675, 202927725, 1217566350, 6358402050, 29248649430, 119653565850, 438729741450, 1451182990950, 4353548972850, 11899700525790, 29749251314475, 68248282427325, 144079707346575, 280576272201225, 505037289962205, 841728816603675, 1300853625660225, 1866442158555975, 2488589544741300, 3085851035479212, 3560597348629860, 3824345300380220, 3824345300380220, 3560597348629860, 3085851035479212, 2488589544741300, 1866442158555975, 1300853625660225, 841728816603675, 505037289962205, 280576272201225, 144079707346575, 68248282427325, 29749251314475, 11899700525790, 4353548972850, 1451182990950, 438729741450, 119653565850, 29248649430, 6358402050, 1217566350, 202927725, 28989675, 3478761, 341055, 26235, 1485, 55, 1
};
// clang-format on

} // anonmyous namespace

class vtkMathInternal : public vtkObjectBase
{
public:
  vtkBaseTypeMacro(vtkMathInternal, vtkObjectBase);
  static vtkMathInternal* New()
  {
    // Can't use object factory macros, since they cast to vtkObject*.
    vtkMathInternal* ret = new vtkMathInternal;
    ret->InitializeObjectBase();
    return ret;
  }

  vtkMinimalStandardRandomSequence* Uniform;
  vtkBoxMuellerRandomSequence* Gaussian;
  std::vector<vtkTypeInt64> MemoizeFactorial;

private:
  vtkMathInternal();
  ~vtkMathInternal() override;
};

vtkMathInternal::vtkMathInternal()
{
  this->Gaussian = vtkBoxMuellerRandomSequence::New();

  // This line assumes the current vtkBoxMuellerRandomSequence behavior:
  // an initial vtkMinimalStandardRandomSequence is created.
  this->Uniform =
    static_cast<vtkMinimalStandardRandomSequence*>(this->Gaussian->GetUniformSequence());
  this->Uniform->SetSeedOnly(1177); // One author's home address
  this->MemoizeFactorial.resize(21, 0);
}

vtkMathInternal::~vtkMathInternal()
{
  this->Gaussian->Delete();
}

vtkSmartPointer<vtkMathInternal> vtkMath::Internal = vtkSmartPointer<vtkMathInternal>::New();

//
// Some useful macros and functions
//

//------------------------------------------------------------------------------
// Return the lowest value "i" for which 2^i >= x
int vtkMath::CeilLog2(vtkTypeUInt64 x)
{
  static const vtkTypeUInt64 t[6] = { 0xffffffff00000000ull, 0x00000000ffff0000ull,
    0x000000000000ff00ull, 0x00000000000000f0ull, 0x000000000000000cull, 0x0000000000000002ull };

  int j = 32;

  // if x is not a power of two, add 1 to final answer
  // (this is the "ceil" part of the computation)
  int y = (((x & (x - 1)) == 0) ? 0 : 1);

  // loop through the table (this unrolls nicely)
  for (int i = 0; i < 6; ++i)
  {
    int k = (((x & t[i]) == 0) ? 0 : j);
    y += k;
    x >>= k;
    j >>= 1;
  }

  return y;
}

double vtkMath::JacobiPolynomial(int nn, double alpha, double beta, double xx)
{
  // Both of the implementations below are correct and within 1e-12 of one
  // another over alpha, beta, xx in [-1,1]³, nn in [0,4].
  // However, option 2 is ~3.3x faster.
#if 0
  // Option 1: Compute via the hypergeometric function.
  double vv =
    std::tgamma(alpha + nn + 1.) /
    std::tgamma(alpha + beta + nn + 1.) /
    static_cast<double>(vtkMath::Factorial(nn));
  double ss = 0.;
  for (int mm = 0; mm <= nn; ++mm)
  {
    double bb = static_cast<double>(vtkMath::Binomial(nn, mm)) *
      std::tgamma(alpha + beta + mm + nn + 1.) /
      std::tgamma(alpha + mm + 1.) *
      std::pow((xx - 1.) / 2., mm);
    ss += bb;
  }
  return ss * vv;
#else
  // Option 2: Compute using binomial expressions and powers of partitions of unity.
  double vv = 0.;
  for (int ss = 0; ss <= nn; ++ss)
  {
    double term = vtkMath::RealBinomial(nn + alpha, nn - ss) *
      vtkMath::RealBinomial(nn + beta, ss) * std::pow(0.5 * (xx - 1), ss) *
      std::pow(0.5 * (xx + 1), nn - ss);
    vv += term;
  }
  return vv;
#endif
}

double vtkMath::JacobiPolynomialDerivative(int nn, double alpha, double beta, double xx)
{
  assert(alpha >= -1.);
  assert(beta >= -1.);
  assert(nn >= 0);
  double result;
  if (nn == 0)
  {
    return 0.;
  }
  double tmp = JacobiPolynomial(nn - 1, alpha + 1, beta + 1, xx);
  result = 0.5 * (nn + alpha + beta + 1.) * tmp;
  return result;
}

//------------------------------------------------------------------------------
// Generate pseudo-random numbers distributed according to the uniform
// distribution between 0.0 and 1.0.
// This is used to provide portability across different systems.
double vtkMath::Random()
{
  vtkMath::Internal->Uniform->Next();
  return vtkMath::Internal->Uniform->GetValue();
}

//------------------------------------------------------------------------------
// Initialize seed value. NOTE: Random() has the bad property that
// the first random number returned after RandomSeed() is called
// is proportional to the seed value! To help solve this, call
// RandomSeed() a few times inside seed. This doesn't ruin the
// repeatability of Random().
void vtkMath::RandomSeed(int s)
{
  vtkMath::Internal->Uniform->SetSeed(s);
}

//------------------------------------------------------------------------------
// Description:
// Return the current seed used by the random number generator.
int vtkMath::GetSeed()
{
  return vtkMath::Internal->Uniform->GetSeed();
}

//------------------------------------------------------------------------------
double vtkMath::Random(double min, double max)
{
  vtkMath::Internal->Uniform->Next();
  return vtkMath::Internal->Uniform->GetRangeValue(min, max);
}

//------------------------------------------------------------------------------
double vtkMath::Gaussian()
{
  vtkMath::Internal->Gaussian->Next();
  return vtkMath::Internal->Gaussian->GetValue();
}

//------------------------------------------------------------------------------
double vtkMath::Gaussian(double mean, double std)
{
  vtkMath::Internal->Gaussian->Next();
  return vtkMath::Internal->Gaussian->GetScaledValue(mean, std);
}

//------------------------------------------------------------------------------
vtkTypeInt64 vtkMath::Factorial(int N)
{
  if (N > 20)
  {
    vtkGenericWarningMacro("Factorial(" << N << ") would overflow.");
    return vtkTypeTraits<vtkTypeInt64>::Max();
  }

  if (N == 0)
  {
    return 1;
  }

  if (vtkMath::Internal->MemoizeFactorial[N] != 0)
  {
    return vtkMath::Internal->MemoizeFactorial[N];
  }

  vtkTypeInt64 r = vtkMath::Factorial(N - 1) * N;
  vtkMath::Internal->MemoizeFactorial[N] = r;
  return r;
}

//------------------------------------------------------------------------------
// The number of combinations of n objects from a pool of m objects (m>n).
vtkTypeInt64 vtkMath::Binomial(int m, int n)
{
  // DoubleBinomial() is exact over the range of its lookup table and
  // accumulates a little rounding error above it, so round rather than
  // truncate. Truncating a running product is what made Binomial(11, 5) return
  // 461 rather than 462, and Binomial(56, 28) one less than its true value.
  return static_cast<vtkTypeInt64>(vtkMath::DoubleBinomial(m, n) + 0.5);
}

double vtkMath::DoubleBinomial(int m, int n)
{
  if (m < 0 || n < 0 || n > m)
  {
    return 0;
  }

  if (m <= 55)
  {
    int offset = (m * (m + 1)) / 2;
    return vtkPascalTriangle[offset + n];
  }

  double r = 1;
  for (int i = 1; i <= n; ++i)
  {
    r *= static_cast<double>(m - i + 1) / i;
  }
  return r;
}

//------------------------------------------------------------------------------
// The extended binomial function that accepts its first parameter as real-valued.
double vtkMath::RealBinomial(double mm, int nn)
{
  double vv = 1.;
  double aa = mm;
  double ff = nn;
  for (int ii = 0; ii < nn; ++ii, ff -= 1, aa -= 1)
  {
    vv *= aa / ff;
  }
  return vv;
}

//------------------------------------------------------------------------------
// Start iterating over "m choose n" objects.
// This function returns an array of n integers, each from 0 to m-1.
// These integers represent the n items chosen from the set [0,m[.
int* vtkMath::BeginCombination(int m, int n)
{
  if (m < n)
  {
    return nullptr;
  }

  int* r = new int[n];
  for (int i = 0; i < n; ++i)
  {
    r[i] = i;
  }
  return r;
}

//------------------------------------------------------------------------------
// Given \a m, \a n, and a valid \a combination of \a n integers in
// the range [0,m[, this function alters the integers into the next
// combination in a sequence of all combinations of \a n items from
// a pool of \a m.
// If the \a combination is the last item in the sequence on input,
// then \a combination is unaltered and 0 is returned.
// Otherwise, 1 is returned and \a combination is updated.
int vtkMath::NextCombination(int m, int n, int* combination)
{
  int status = 0;
  for (int i = n - 1; i >= 0; --i)
  {
    if (combination[i] < m - n + i)
    {
      int j = combination[i] + 1;
      while (i < n)
      {
        combination[i++] = j++;
      }
      status = 1;
      break;
    }
  }
  return status;
}

//------------------------------------------------------------------------------
// Free the "iterator" array created by vtkMath::BeginCombination.
//
void vtkMath::FreeCombination(int* combination)
{
  delete[] combination;
}

//------------------------------------------------------------------------------
// Given a unit vector v1, find two other unit vectors v2 and v3 which
// which form an orthonormal set.
template <class T1, class T2, class T3>
inline void vtkMathPerpendiculars(const T1 v1[3], T2 v2[3], T3 v3[3], double theta)
{
  double v1sq = v1[0] * v1[0];
  double v2sq = v1[1] * v1[1];
  double v3sq = v1[2] * v1[2];
  double r = std::sqrt(v1sq + v2sq + v3sq);

  // transpose the vector to avoid divide-by-zero error
  int dv1, dv2, dv3;
  if (v1sq > v2sq && v1sq > v3sq)
  {
    dv1 = 0;
    dv2 = 1;
    dv3 = 2;
  }
  else if (v2sq > v3sq)
  {
    dv1 = 1;
    dv2 = 2;
    dv3 = 0;
  }
  else
  {
    dv1 = 2;
    dv2 = 0;
    dv3 = 1;
  }

  double a = v1[dv1] / r;
  double b = v1[dv2] / r;
  double c = v1[dv3] / r;

  double tmp = std::sqrt(a * a + c * c);

  if (theta != 0.0)
  {
    double sintheta = sin(theta);
    double costheta = cos(theta);

    if (v2)
    {
      v2[dv1] = (c * costheta - a * b * sintheta) / tmp;
      v2[dv2] = sintheta * tmp;
      v2[dv3] = (-a * costheta - b * c * sintheta) / tmp;
    }

    if (v3)
    {
      v3[dv1] = (-c * sintheta - a * b * costheta) / tmp;
      v3[dv2] = costheta * tmp;
      v3[dv3] = (a * sintheta - b * c * costheta) / tmp;
    }
  }
  else
  {
    if (v2)
    {
      v2[dv1] = c / tmp;
      v2[dv2] = 0;
      v2[dv3] = -a / tmp;
    }

    if (v3)
    {
      v3[dv1] = -a * b / tmp;
      v3[dv2] = tmp;
      v3[dv3] = -b * c / tmp;
    }
  }
}

void vtkMath::Perpendiculars(const double v1[3], double v2[3], double v3[3], double theta)
{
  vtkMathPerpendiculars(v1, v2, v3, theta);
}

void vtkMath::Perpendiculars(const float v1[3], float v2[3], float v3[3], double theta)
{
  vtkMathPerpendiculars(v1, v2, v3, theta);
}

//------------------------------------------------------------------------------
// Solve linear equation Ax = b using Gaussian Elimination with Partial Pivoting
// for a 2x2 system. If the matrix is found to be singular within a small numerical
// tolerance close to machine precision then 0 is returned.
vtkTypeBool vtkMath::SolveLinearSystemGEPP2x2(
  double a00, double a01, double a10, double a11, double b0, double b1, double& x0, double& x1)
{
  // Check if any of the matrix coefficients is zero.
  // If so then swap rows/columns to form an upper triangular matrix without
  // having to use GEPP.
  bool cols_swapped = false;
  if ((a00 == 0) || (a01 == 0) || (a10 == 0) || (a11 == 0))
  {
    // zero in either row of the 2nd column?
    if ((a01 == 0) || (a11 == 0))
    {
      // swap columns
      std::swap(a00, a01);
      std::swap(a10, a11);
      cols_swapped = true;
    }
    // zero in a00?
    if (a00 == 0)
    {
      // swap rows
      std::swap(a00, a10);
      std::swap(a01, a11);
      std::swap(b0, b1);
    }
  }
  else
  {
    // None of the matrix coefficients are exactly zero.
    // Use GEPP to form upper triangular matrix, i.e. so that a10 == 0.
    // Select pivot by looking at largest absolute value in a00, a10
    if (std::abs(a00) < std::abs(a10))
    {
      // swap rows so largest coefficient in first column is in the first row
      std::swap(a00, a10);
      std::swap(a01, a11);
      std::swap(b0, b1);
    }
    // a10 = 0;            // bookkeeping only, value is no longer required
    const double f = -a10 / a00;
    a11 += a01 * f;
    b1 += b0 * f;
  }
  // Have now an exact zero in a10.
  // Need to check for singularity by looking at a11.
  // Note the choice of eps is reasonable but somewhat arbitrary.
  static const double eps = 256 * std::numeric_limits<double>::epsilon();
  if (std::abs(a11) < eps)
  {
    // matrix is singular within small numerical tolerance
    return 0;
  }
  // Solve the triangular system
  if (a11 != 0)
  {
    x1 = b1 / a11;
  }
  else
  {
    return 0;
  }
  if (a00 != 0)
  {
    x0 = (b0 - a01 * x1) / a00;
  }
  else
  {
    return 0;
  }
  // other failures in solution?
  if (!std::isfinite(x0) || !std::isfinite(x1))
  {
    return 0;
  }
  // If necessary swap solution vector rows.
  if (cols_swapped)
  {
    std::swap(x0, x1);
  }
  return 1;
}

namespace
{
constexpr double VTK_SMALL_NUMBER = 1.0e-12;
constexpr int VTK_MAX_SCRATCH_ARRAY_SIZE = 10;
constexpr int VTK_MAX_ROTATIONS = 20;
}

//------------------------------------------------------------------------------
// Solve linear equations Ax = b using Crout's method. Input is square matrix A
// and load vector b. Solution x is written over load vector. The dimension of
// the matrix is specified in size. If error is found, method returns a 0.
vtkTypeBool vtkMath::SolveLinearSystem(double** A, double* x, int size)
{
  // if we solving something simple, just solve it
  //
  if (size == 2)
  {
    return SolveLinearSystemGEPP2x2(A[0][0], A[0][1], A[1][0], A[1][1], x[0], x[1], x[0], x[1]);
  }
  else if (size == 1)
  {
    if (A[0][0] == 0.0)
    {
      // Unable to solve linear system
      return 0;
    }

    x[0] /= A[0][0];
    return 1;
  }

  //
  // System of equations is not trivial, use Crout's method
  //

  // Check on allocation of working vectors
  //
  int *index, scratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
  index = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? scratch : new int[size]);

  //
  // Factor and solve matrix
  //
  if (vtkMath::LUFactorLinearSystem(A, index, size) == 0)
  {
    return 0;
  }
  vtkMath::LUSolveLinearSystem(A, index, x, size);

  if (size > VTK_MAX_SCRATCH_ARRAY_SIZE)
  {
    delete[] index;
  }
  return 1;
}

//------------------------------------------------------------------------------
// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
vtkTypeBool vtkMath::InvertMatrix(double** A, double** AI, int size)
{
  int iScratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
  int* index = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? iScratch : new int[size]);
  double dScratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
  double* column = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? dScratch : new double[size]);

  vtkTypeBool retVal = vtkMath::InvertMatrix(A, AI, size, index, column);

  if (size > VTK_MAX_SCRATCH_ARRAY_SIZE)
  {
    delete[] index;
    delete[] column;
  }

  return retVal;
}

//------------------------------------------------------------------------------
// Factor linear equations Ax = b using LU decomposition A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is
// found, method returns 0.
vtkTypeBool vtkMath::LUFactorLinearSystem(double** A, int* index, int size)
{
  double scratch[VTK_MAX_SCRATCH_ARRAY_SIZE];
  double* scale = (size <= VTK_MAX_SCRATCH_ARRAY_SIZE ? scratch : new double[size]);

  int i, j, k;
  int maxI = 0;
  double largest, temp1, temp2, sum;

  //
  // Loop over rows to get implicit scaling information
  //
  for (i = 0; i < size; ++i)
  {
    for (largest = 0.0, j = 0; j < size; ++j)
    {
      if ((temp2 = std::abs(A[i][j])) > largest)
      {
        largest = temp2;
      }
    }

    if (largest == 0.0)
    {
      vtkGenericWarningMacro(<< "Unable to factor linear system");
      if (size > VTK_MAX_SCRATCH_ARRAY_SIZE)
      {
        delete[] scale;
      }
      return 0;
    }
    scale[i] = 1.0 / largest;
  }
  //
  // Loop over all columns using Crout's method
  //
  for (j = 0; j < size; ++j)
  {
    for (i = 0; i < j; ++i)
    {
      sum = A[i][j];
      for (k = 0; k < i; ++k)
      {
        sum -= A[i][k] * A[k][j];
      }
      A[i][j] = sum;
    }
    //
    // Begin search for largest pivot element
    //
    for (largest = 0.0, i = j; i < size; ++i)
    {
      sum = A[i][j];
      for (k = 0; k < j; ++k)
      {
        sum -= A[i][k] * A[k][j];
      }
      A[i][j] = sum;

      if ((temp1 = scale[i] * std::abs(sum)) >= largest)
      {
        largest = temp1;
        maxI = i;
      }
    }
    //
    // Check for row interchange
    //
    if (j != maxI)
    {
      for (k = 0; k < size; ++k)
      {
        temp1 = A[maxI][k];
        A[maxI][k] = A[j][k];
        A[j][k] = temp1;
      }
      scale[maxI] = scale[j];
    }
    //
    // Divide by pivot element and perform elimination
    //
    index[j] = maxI;

    if (std::abs(A[j][j]) <= VTK_SMALL_NUMBER)
    {
      vtkGenericWarningMacro(<< "Unable to factor linear system");
      if (size > VTK_MAX_SCRATCH_ARRAY_SIZE)
      {
        delete[] scale;
      }
      return 0;
    }

    if (j != (size - 1))
    {
      temp1 = 1.0 / A[j][j];
      for (i = j + 1; i < size; ++i)
      {
        A[i][j] *= temp1;
      }
    }
  }

  if (size > VTK_MAX_SCRATCH_ARRAY_SIZE)
  {
    delete[] scale;
  }

  return 1;
}

//------------------------------------------------------------------------------
// Solve linear equations Ax = b using LU decomposition A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is
// factored matrix A=LU, integer array of pivot indices index[0->n-1],
// load vector x[0->n-1], and size of square matrix n. Note that A=LU and
// index[] are generated from method LUFactorLinearSystem). Also, solution
// vector is written directly over input load vector.
void vtkMath::LUSolveLinearSystem(double** A, int* index, double* x, int size)
{
  int i, j, ii, idx;
  double sum;
  //
  // Proceed with forward and backsubstitution for L and U
  // matrices.  First, forward substitution.
  //
  for (ii = -1, i = 0; i < size; ++i)
  {
    idx = index[i];
    sum = x[idx];
    x[idx] = x[i];

    if (ii >= 0)
    {
      for (j = ii; j <= (i - 1); ++j)
      {
        sum -= A[i][j] * x[j];
      }
    }
    else if (sum != 0.0)
    {
      ii = i;
    }

    x[i] = sum;
  }
  //
  // Now, back substitution
  //
  for (i = size - 1; i >= 0; i--)
  {
    sum = x[i];
    for (j = i + 1; j < size; ++j)
    {
      sum -= A[i][j] * x[j];
    }
    x[i] = sum / A[i][i];
  }
}

#define VTK_ROTATE(a, i, j, k, l)                                                                  \
  g = a[i][j];                                                                                     \
  h = a[k][l];                                                                                     \
  a[i][j] = g - s * (h + g * tau);                                                                 \
  a[k][l] = h + s * (g - h * tau)

// Jacobi iteration for the solution of eigenvectors/eigenvalues of a nxn
// real symmetric matrix. Square nxn matrix a; size of matrix in n;
// output eigenvalues in w; and output eigenvectors in v. Resulting
// eigenvalues/vectors are sorted in decreasing order; eigenvectors are
// normalized.
// It assumes a is symmetric and uses only its upper right triangular part.
template <class T>
vtkTypeBool vtkJacobiN(T** a, int n, T* w, T** v)
{
  int i, j, k, iq, ip, numPos;
  T tresh, theta, tau, t, sm, s, h, g, c, tmp;
  T bspace[VTK_MAX_SCRATCH_ARRAY_SIZE], zspace[VTK_MAX_SCRATCH_ARRAY_SIZE];
  T* b = (n <= VTK_MAX_SCRATCH_ARRAY_SIZE) ? bspace : new T[n];
  T* z = (n <= VTK_MAX_SCRATCH_ARRAY_SIZE) ? zspace : new T[n];

  // initialize
  for (ip = 0; ip < n; ip++)
  {
    for (iq = 0; iq < n; iq++)
    {
      v[ip][iq] = 0.0;
    }
    v[ip][ip] = 1.0;
  }
  for (ip = 0; ip < n; ip++)
  {
    b[ip] = w[ip] = a[ip][ip];
    z[ip] = 0.0;
  }

  // begin rotation sequence
  for (i = 0; i < VTK_MAX_ROTATIONS; ++i)
  {
    sm = 0.0;
    for (ip = 0; ip < n - 1; ip++)
    {
      for (iq = ip + 1; iq < n; iq++)
      {
        sm += std::abs(a[ip][iq]);
      }
    }
    if (sm == 0.0)
    {
      break;
    }

    if (i < 3) // first 3 sweeps
    {
      tresh = 0.2 * sm / (n * n);
    }
    else
    {
      tresh = 0.0;
    }

    for (ip = 0; ip < n - 1; ip++)
    {
      for (iq = ip + 1; iq < n; iq++)
      {
        g = 100.0 * std::abs(a[ip][iq]);

        // after 4 sweeps
        if (i > 3 && (std::abs(w[ip]) + g) == std::abs(w[ip]) &&
          (std::abs(w[iq]) + g) == std::abs(w[iq]))
        {
          a[ip][iq] = 0.0;
        }
        else if (std::abs(a[ip][iq]) > tresh)
        {
          h = w[iq] - w[ip];
          if ((std::abs(h) + g) == std::abs(h))
          {
            t = (a[ip][iq]) / h;
          }
          else
          {
            theta = 0.5 * h / (a[ip][iq]);
            t = 1.0 / (std::abs(theta) + std::sqrt(1.0 + theta * theta));
            if (theta < 0.0)
            {
              t = -t;
            }
          }
          c = 1.0 / std::sqrt(1 + t * t);
          s = t * c;
          tau = s / (1.0 + c);
          h = t * a[ip][iq];
          z[ip] -= h;
          z[iq] += h;
          w[ip] -= h;
          w[iq] += h;
          a[ip][iq] = 0.0;

          // ip already shifted left by 1 unit
          for (j = 0; j <= ip - 1; ++j)
          {
            VTK_ROTATE(a, j, ip, j, iq);
          }
          // ip and iq already shifted left by 1 unit
          for (j = ip + 1; j <= iq - 1; ++j)
          {
            VTK_ROTATE(a, ip, j, j, iq);
          }
          // iq already shifted left by 1 unit
          for (j = iq + 1; j < n; ++j)
          {
            VTK_ROTATE(a, ip, j, iq, j);
          }
          for (j = 0; j < n; ++j)
          {
            VTK_ROTATE(v, j, ip, j, iq);
          }
        }
      }
    }

    for (ip = 0; ip < n; ip++)
    {
      b[ip] += z[ip];
      w[ip] = b[ip];
      z[ip] = 0.0;
    }
  }

  //// this is NEVER called
  if (i >= VTK_MAX_ROTATIONS)
  {
    vtkGenericWarningMacro("vtkMath::Jacobi: Error extracting eigenfunctions");
    if (n > VTK_MAX_SCRATCH_ARRAY_SIZE)
    {
      delete[] b;
      delete[] z;
    }
    return 0;
  }

  // sort eigenfunctions                 these changes do not affect accuracy
  for (j = 0; j < n - 1; ++j) // boundary incorrect
  {
    k = j;
    tmp = w[k];
    for (i = j + 1; i < n; ++i) // boundary incorrect, shifted already
    {
      if (w[i] >= tmp) // why exchange if same?
      {
        k = i;
        tmp = w[k];
      }
    }
    if (k != j)
    {
      w[k] = w[j];
      w[j] = tmp;
      for (i = 0; i < n; ++i)
      {
        tmp = v[i][j];
        v[i][j] = v[i][k];
        v[i][k] = tmp;
      }
    }
  }
  // ensure eigenvector consistency (i.e., Jacobi can compute vectors that
  // are negative of one another (.707,.707,0) and (-.707,-.707,0). This can
  // reek havoc in hyperstreamline/other stuff. We will select the most
  // positive eigenvector.
  int ceil_half_n = (n >> 1) + (n & 1);
  for (j = 0; j < n; ++j)
  {
    for (numPos = 0, i = 0; i < n; ++i)
    {
      if (v[i][j] >= 0.0)
      {
        numPos++;
      }
    }
    if (numPos < ceil_half_n)
    {
      for (i = 0; i < n; ++i)
      {
        v[i][j] *= -1.0;
      }
    }
  }

  if (n > VTK_MAX_SCRATCH_ARRAY_SIZE)
  {
    delete[] b;
    delete[] z;
  }
  return 1;
}

#undef VTK_ROTATE

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::JacobiN(float** a, int n, float* w, float** v)
{
  return vtkJacobiN(a, n, w, v);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::JacobiN(double** a, int n, double* w, double** v)
{
  return vtkJacobiN(a, n, w, v);
}

//------------------------------------------------------------------------------
// Jacobi iteration for the solution of eigenvectors/eigenvalues of a 3x3
// real symmetric matrix. Square 3x3 matrix a; output eigenvalues in w;
// and output eigenvectors in v. Resulting eigenvalues/vectors are sorted
// in decreasing order; eigenvectors are normalized.
vtkTypeBool vtkMath::Jacobi(float** a, float* w, float** v)
{
  return vtkMath::JacobiN(a, 3, w, v);
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::Jacobi(double** a, double* w, double** v)
{
  return vtkMath::JacobiN(a, 3, w, v);
}

//------------------------------------------------------------------------------
// Estimate the condition number of a LU factored matrix. Used to judge the
// accuracy of the solution. The matrix A must have been previously factored
// using the method LUFactorLinearSystem. The condition number is the ratio
// of the infinity matrix norm (i.e., maximum value of matrix component)
// divided by the minimum diagonal value. (This works for triangular matrices
// only: see Conte and de Boor, Elementary Numerical Analysis.)
double vtkMath::EstimateMatrixCondition(const double* const* A, int size)
{
  int i;
  int j;
  double min = VTK_FLOAT_MAX, max = (-VTK_FLOAT_MAX);

  // find the maximum value
  for (i = 0; i < size; ++i)
  {
    for (j = i; j < size; ++j)
    {
      max = std::max(std::abs(A[i][j]), max);
    }
  }

  // find the minimum diagonal value
  for (i = 0; i < size; ++i)
  {
    min = std::min(std::abs(A[i][i]), min);
  }

  if (min == 0.0)
  {
    return VTK_FLOAT_MAX;
  }
  else
  {
    return (max / min);
  }
}

//------------------------------------------------------------------------------
// Solves for the least squares best fit matrix for the homogeneous equation X'M' = 0'.
// Uses the method described on pages 40-41 of Computer Vision by
// Forsyth and Ponce, which is that the solution is the eigenvector
// associated with the minimum eigenvalue of T(X)X, where T(X) is the
// transpose of X.
// The inputs and output are transposed matrices.
//    Dimensions: X' is numberOfSamples by xOrder,
//                M' dimension is xOrder by 1.
// M' should be pre-allocated. All matrices are row major. The resultant
// matrix M' should be pre-multiplied to X' to get 0', or transposed and
// then post multiplied to X to get 0.
// Returns success/fail.
vtkTypeBool vtkMath::SolveHomogeneousLeastSquares(
  int numberOfSamples, double** xt, int xOrder, double** mt)
{
  // check dimensional consistency
  if (numberOfSamples < xOrder)
  {
    vtkGenericWarningMacro("Insufficient number of samples. Underdetermined.");
    return 0;
  }

  // set up intermediate variables
  // Allocate matrix to hold X times transpose of X
  double** XXt = new double*[xOrder]; // size x by x
  // Allocate the array of eigenvalues and eigenvectors
  double* eigenvals = new double[xOrder];
  double** eigenvecs = new double*[xOrder];

  int i, j, k;

  // Clear the upper triangular region (and btw, allocate the eigenvecs as well)
  for (i = 0; i < xOrder; ++i)
  {
    eigenvecs[i] = new double[xOrder];
    XXt[i] = new double[xOrder];
    for (j = 0; j < xOrder; ++j)
    {
      XXt[i][j] = 0.0;
    }
  }

  // Calculate XXt upper half only, due to symmetry
  for (k = 0; k < numberOfSamples; ++k)
  {
    for (i = 0; i < xOrder; ++i)
    {
      for (j = i; j < xOrder; ++j)
      {
        XXt[i][j] += xt[k][i] * xt[k][j];
      }
    }
  }

  // now fill in the lower half of the XXt matrix
  for (i = 0; i < xOrder; ++i)
  {
    for (j = 0; j < i; ++j)
    {
      XXt[i][j] = XXt[j][i];
    }
  }

  // Compute the eigenvectors and eigenvalues
  vtkMath::JacobiN(XXt, xOrder, eigenvals, eigenvecs);

  // Smallest eigenval is at the end of the list (xOrder-1), and solution is
  // corresponding eigenvec.
  for (i = 0; i < xOrder; ++i)
  {
    mt[i][0] = eigenvecs[i][xOrder - 1];
  }

  // Clean up:
  for (i = 0; i < xOrder; ++i)
  {
    delete[] XXt[i];
    delete[] eigenvecs[i];
  }
  delete[] XXt;
  delete[] eigenvecs;
  delete[] eigenvals;

  return 1;
}

//------------------------------------------------------------------------------
// Solves for the least squares best fit matrix for the equation X'M' = Y'.
// Uses pseudoinverse to get the ordinary least squares.
// The inputs and output are transposed matrices.
//    Dimensions: X' is numberOfSamples by xOrder,
//                Y' is numberOfSamples by yOrder,
//                M' dimension is xOrder by yOrder.
// M' should be pre-allocated. All matrices are row major. The resultant
// matrix M' should be pre-multiplied to X' to get Y', or transposed and
// then post multiplied to X to get Y
// By default, this method checks for the homogeneous condition where Y==0, and
// if so, invokes SolveHomogeneousLeastSquares. For better performance when
// the system is known not to be homogeneous, invoke with checkHomogeneous=0.
// Returns success/fail.
vtkTypeBool vtkMath::SolveLeastSquares(int numberOfSamples, double** xt, int xOrder, double** yt,
  int yOrder, double** mt, int checkHomogeneous)
{
  // check dimensional consistency
  if ((numberOfSamples < xOrder) || (numberOfSamples < yOrder))
  {
    vtkGenericWarningMacro("Insufficient number of samples. Underdetermined.");
    return 0;
  }

  int i, j, k;

  bool someHomogeneous = false;
  bool allHomogeneous = true;
  double** hmt = nullptr;
  vtkTypeBool homogRC = 0;
  int* homogenFlags = new int[yOrder];
  vtkTypeBool successFlag;

  // Ok, first init some flags check and see if all the systems are homogeneous
  if (checkHomogeneous)
  {
    // If Y' is zero, it's a homogeneous system and can't be solved via
    // the pseudoinverse method. Detect this case, warn the user, and
    // invoke SolveHomogeneousLeastSquares instead. Note that it doesn't
    // really make much sense for yOrder to be greater than one in this case,
    // since that's just yOrder occurrences of a 0 vector on the RHS, but
    // we allow it anyway. N

    // Initialize homogeneous flags on a per-right-hand-side basis
    for (j = 0; j < yOrder; ++j)
    {
      homogenFlags[j] = 1;
    }
    for (i = 0; i < numberOfSamples; ++i)
    {
      for (j = 0; j < yOrder; ++j)
      {
        if (std::abs(yt[i][j]) > VTK_SMALL_NUMBER)
        {
          allHomogeneous = false;
          homogenFlags[j] = 0;
        }
      }
    }

    // If we've got one system, and it's homogeneous, do it and bail out quickly.
    if (allHomogeneous && yOrder == 1)
    {
      vtkGenericWarningMacro(
        "Detected homogeneous system (Y=0), calling SolveHomogeneousLeastSquares()");
      delete[] homogenFlags;
      return vtkMath::SolveHomogeneousLeastSquares(numberOfSamples, xt, xOrder, mt);
    }

    // Ok, we've got more than one system of equations.
    // Figure out if we need to calculate the homogeneous equation solution for
    // any of them.
    if (allHomogeneous)
    {
      someHomogeneous = true;
    }
    else
    {
      for (j = 0; j < yOrder; ++j)
      {
        if (homogenFlags[j])
        {
          someHomogeneous = true;
        }
      }
    }
  }

  // If necessary, solve the homogeneous problem
  if (someHomogeneous)
  {
    // hmt is the homogeneous equation version of mt, the general solution.
    hmt = new double*[xOrder];
    for (j = 0; j < xOrder; ++j)
    {
      // Only allocate 1 here, not yOrder, because here we're going to solve
      // just the one homogeneous equation subset of the entire problem
      hmt[j] = new double[1];
    }

    // Ok, solve the homogeneous problem
    homogRC = vtkMath::SolveHomogeneousLeastSquares(numberOfSamples, xt, xOrder, hmt);
  }

  // set up intermediate variables
  double** XXt = new double*[xOrder];  // size x by x
  double** XXtI = new double*[xOrder]; // size x by x
  double** XYt = new double*[xOrder];  // size x by y
  for (i = 0; i < xOrder; ++i)
  {
    XXt[i] = new double[xOrder];
    XXtI[i] = new double[xOrder];

    for (j = 0; j < xOrder; ++j)
    {
      XXt[i][j] = 0.0;
      XXtI[i][j] = 0.0;
    }

    XYt[i] = new double[yOrder];
    for (j = 0; j < yOrder; ++j)
    {
      XYt[i][j] = 0.0;
    }
  }

  // first find the pseudoinverse matrix
  for (k = 0; k < numberOfSamples; ++k)
  {
    for (i = 0; i < xOrder; ++i)
    {
      // first calculate the XXt matrix, only do the upper half (symmetrical)
      for (j = i; j < xOrder; ++j)
      {
        XXt[i][j] += xt[k][i] * xt[k][j];
      }

      // now calculate the XYt matrix
      for (j = 0; j < yOrder; ++j)
      {
        XYt[i][j] += xt[k][i] * yt[k][j];
      }
    }
  }

  // now fill in the lower half of the XXt matrix
  for (i = 0; i < xOrder; ++i)
  {
    for (j = 0; j < i; ++j)
    {
      XXt[i][j] = XXt[j][i];
    }
  }

  successFlag = vtkMath::InvertMatrix(XXt, XXtI, xOrder);

  // next get the inverse of XXt
  if (successFlag)
  {
    for (i = 0; i < xOrder; ++i)
    {
      for (j = 0; j < yOrder; ++j)
      {
        mt[i][j] = 0.0;
        for (k = 0; k < xOrder; ++k)
        {
          mt[i][j] += XXtI[i][k] * XYt[k][j];
        }
      }
    }
  }

  // Fix up any of the solutions that correspond to the homogeneous equation
  // problem.
  if (someHomogeneous)
  {
    for (j = 0; j < yOrder; ++j)
    {
      if (homogenFlags[j])
      {
        // Fix this one
        for (i = 0; i < xOrder; ++i)
        {
          mt[i][j] = hmt[i][0];
        }
      }
    }

    // Clean up
    for (i = 0; i < xOrder; ++i)
    {
      delete[] hmt[i];
    }
    delete[] hmt;
  }

  // clean up:
  // set up intermediate variables
  for (i = 0; i < xOrder; ++i)
  {
    delete[] XXt[i];
    delete[] XXtI[i];

    delete[] XYt[i];
  }
  delete[] XXt;
  delete[] XXtI;
  delete[] XYt;
  delete[] homogenFlags;

  if (someHomogeneous)
  {
    return homogRC && successFlag;
  }
  else
  {
    return successFlag;
  }
}

//=============================================================================
// Thread safe versions of math methods.
//=============================================================================

// Invert input square matrix A into matrix AI. Note that A is modified during
// the inversion. The size variable is the dimension of the matrix. Returns 0
// if inverse not computed.
// -----------------------
// For thread safe behavior, temporary arrays tmp1SIze and tmp2Size
// of length size must be passed in.
vtkTypeBool vtkMath::InvertMatrix(
  double** A, double** AI, int size, int* tmp1Size, double* tmp2Size)
{
  int i, j;

  //
  // Factor matrix; then begin solving for inverse one column at a time.
  // Note: tmp1Size returned value is used later, tmp2Size is just working
  // memory whose values are not used in LUSolveLinearSystem
  //
  if (vtkMath::LUFactorLinearSystem(A, tmp1Size, size, tmp2Size) == 0)
  {
    return 0;
  }

  for (j = 0; j < size; ++j)
  {
    for (i = 0; i < size; ++i)
    {
      tmp2Size[i] = 0.0;
    }
    tmp2Size[j] = 1.0;

    vtkMath::LUSolveLinearSystem(A, tmp1Size, tmp2Size, size);

    for (i = 0; i < size; ++i)
    {
      AI[i][j] = tmp2Size[i];
    }
  }

  return 1;
}

// Factor linear equations Ax = b using LU decomposition A = LU where L is
// lower triangular matrix and U is upper triangular matrix. Input is
// square matrix A, integer array of pivot indices index[0->n-1], and size
// of square matrix n. Output factorization LU is in matrix A. If error is
// found, method returns 0.
//------------------------------------------------------------------
// For thread safe, temporary memory array tmpSize of length size
// must be passed in.
vtkTypeBool vtkMath::LUFactorLinearSystem(double** A, int* index, int size, double* tmpSize)
{
  int i, j, k;
  int maxI = 0;
  double largest, temp1, temp2, sum;

  //
  // Loop over rows to get implicit scaling information
  //
  for (i = 0; i < size; ++i)
  {
    for (largest = 0.0, j = 0; j < size; ++j)
    {
      if ((temp2 = std::abs(A[i][j])) > largest)
      {
        largest = temp2;
      }
    }

    if (largest == 0.0)
    {
      vtkGenericWarningMacro(<< "Unable to factor linear system");
      return 0;
    }
    tmpSize[i] = 1.0 / largest;
  }
  //
  // Loop over all columns using Crout's method
  //
  for (j = 0; j < size; ++j)
  {
    for (i = 0; i < j; ++i)
    {
      sum = A[i][j];
      for (k = 0; k < i; ++k)
      {
        sum -= A[i][k] * A[k][j];
      }
      A[i][j] = sum;
    }
    //
    // Begin search for largest pivot element
    //
    for (largest = 0.0, i = j; i < size; ++i)
    {
      sum = A[i][j];
      for (k = 0; k < j; ++k)
      {
        sum -= A[i][k] * A[k][j];
      }
      A[i][j] = sum;

      if ((temp1 = tmpSize[i] * std::abs(sum)) >= largest)
      {
        largest = temp1;
        maxI = i;
      }
    }
    //
    // Check for row interchange
    //
    if (j != maxI)
    {
      for (k = 0; k < size; ++k)
      {
        std::swap(A[maxI][k], A[j][k]);
      }
      tmpSize[maxI] = tmpSize[j];
    }
    //
    // Divide by pivot element and perform elimination
    //
    index[j] = maxI;

    if (std::abs(A[j][j]) <= VTK_SMALL_NUMBER)
    {
      vtkGenericWarningMacro(<< "Unable to factor linear system");
      return 0;
    }

    if (j != (size - 1))
    {
      temp1 = 1.0 / A[j][j];
      for (i = j + 1; i < size; ++i)
      {
        A[i][j] *= temp1;
      }
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// All of the following methods are for dealing with 3x3 matrices
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// helper function, swap two 3-vectors
template <class T>
inline void vtkSwapVectors3(T v1[3], T v2[3])
{
  for (int i = 0; i < 3; ++i)
  {
    std::swap(v1[i], v2[i]);
  }
}

//------------------------------------------------------------------------------
// Unrolled LU factorization of a 3x3 matrix with pivoting.
template <class T>
inline void vtkLUFactor3x3(T A[3][3], int index[3])
{
  int i, maxI;
  T tmp, largest;
  T scale[3];

  // Loop over rows to get implicit scaling information

  for (i = 0; i < 3; ++i)
  {
    largest = std::abs(A[i][0]);
    if ((tmp = std::abs(A[i][1])) > largest)
    {
      largest = tmp;
    }
    if ((tmp = std::abs(A[i][2])) > largest)
    {
      largest = tmp;
    }
    scale[i] = T(1.0) / largest;
  }

  // Loop over all columns using Crout's method

  // first column
  largest = scale[0] * std::abs(A[0][0]);
  maxI = 0;
  if ((tmp = scale[1] * std::abs(A[1][0])) >= largest)
  {
    largest = tmp;
    maxI = 1;
  }
  if ((tmp = scale[2] * std::abs(A[2][0])) >= largest)
  {
    maxI = 2;
  }
  if (maxI != 0)
  {
    vtkSwapVectors3(A[maxI], A[0]);
    scale[maxI] = scale[0];
  }
  index[0] = maxI;

  A[1][0] /= A[0][0];
  A[2][0] /= A[0][0];

  // second column
  A[1][1] -= A[1][0] * A[0][1];
  A[2][1] -= A[2][0] * A[0][1];
  largest = scale[1] * std::abs(A[1][1]);
  maxI = 1;
  if ((tmp = scale[2] * std::abs(A[2][1])) >= largest)
  {
    maxI = 2;
    vtkSwapVectors3(A[2], A[1]);
    scale[2] = scale[1];
  }
  index[1] = maxI;
  A[2][1] /= A[1][1];

  // third column
  A[1][2] -= A[1][0] * A[0][2];
  A[2][2] -= A[2][0] * A[0][2] + A[2][1] * A[1][2];
  index[2] = 2;
}

//------------------------------------------------------------------------------
void vtkMath::LUFactor3x3(float A[3][3], int index[3])
{
  vtkLUFactor3x3(A, index);
}

//------------------------------------------------------------------------------
void vtkMath::LUFactor3x3(double A[3][3], int index[3])
{
  vtkLUFactor3x3(A, index);
}

//------------------------------------------------------------------------------
// Backsubstitution with an LU-decomposed matrix.
template <class T1, class T2>
inline void vtkLUSolve3x3(const T1 A[3][3], const int index[3], T2 x[3])
{
  T2 sum;

  // forward substitution

  sum = x[index[0]];
  x[index[0]] = x[0];
  x[0] = sum;

  sum = x[index[1]];
  x[index[1]] = x[1];
  x[1] = sum - A[1][0] * x[0];

  sum = x[index[2]];
  x[index[2]] = x[2];
  x[2] = sum - A[2][0] * x[0] - A[2][1] * x[1];

  // back substitution

  x[2] = x[2] / A[2][2];
  x[1] = (x[1] - A[1][2] * x[2]) / A[1][1];
  x[0] = (x[0] - A[0][1] * x[1] - A[0][2] * x[2]) / A[0][0];
}

//------------------------------------------------------------------------------
void vtkMath::LUSolve3x3(const float A[3][3], const int index[3], float x[3])
{
  vtkLUSolve3x3(A, index, x);
}

//------------------------------------------------------------------------------
void vtkMath::LUSolve3x3(const double A[3][3], const int index[3], double x[3])
{
  vtkLUSolve3x3(A, index, x);
}

//------------------------------------------------------------------------------
// this method solves Ay = x for y
template <class T1, class T2, class T3>
inline void vtkLinearSolve3x3(const T1 A[3][3], const T2 x[3], T3 y[3])
{
  double a1 = A[0][0];
  double b1 = A[0][1];
  double c1 = A[0][2];
  double a2 = A[1][0];
  double b2 = A[1][1];
  double c2 = A[1][2];
  double a3 = A[2][0];
  double b3 = A[2][1];
  double c3 = A[2][2];

  // Compute the adjoint
  double d1 = vtkMath::Determinant2x2(b2, b3, c2, c3);
  double d2 = -vtkMath::Determinant2x2(a2, a3, c2, c3);
  double d3 = vtkMath::Determinant2x2(a2, a3, b2, b3);

  double e1 = -vtkMath::Determinant2x2(b1, b3, c1, c3);
  double e2 = vtkMath::Determinant2x2(a1, a3, c1, c3);
  double e3 = -vtkMath::Determinant2x2(a1, a3, b1, b3);

  double f1 = vtkMath::Determinant2x2(b1, b2, c1, c2);
  double f2 = -vtkMath::Determinant2x2(a1, a2, c1, c2);
  double f3 = vtkMath::Determinant2x2(a1, a2, b1, b2);

  // Compute the determinant
  double det = a1 * d1 + b1 * d2 + c1 * d3;

  // Multiply by the adjoint
  double v1 = d1 * x[0] + e1 * x[1] + f1 * x[2];
  double v2 = d2 * x[0] + e2 * x[1] + f2 * x[2];
  double v3 = d3 * x[0] + e3 * x[1] + f3 * x[2];

  // Divide by the determinant
  y[0] = v1 / det;
  y[1] = v2 / det;
  y[2] = v3 / det;
}

//------------------------------------------------------------------------------
void vtkMath::LinearSolve3x3(const float A[3][3], const float x[3], float y[3])
{
  vtkLinearSolve3x3(A, x, y);
}

//------------------------------------------------------------------------------
void vtkMath::LinearSolve3x3(const double A[3][3], const double x[3], double y[3])
{
  vtkLinearSolve3x3(A, x, y);
}

//------------------------------------------------------------------------------
template <class T1, class T2, class T3>
inline void vtkMultiply3x3(const T1 A[3][3], const T2 v[3], T3 u[3])
{
  T3 x = A[0][0] * v[0] + A[0][1] * v[1] + A[0][2] * v[2];
  T3 y = A[1][0] * v[0] + A[1][1] * v[1] + A[1][2] * v[2];
  T3 z = A[2][0] * v[0] + A[2][1] * v[1] + A[2][2] * v[2];

  u[0] = x;
  u[1] = y;
  u[2] = z;
}

//------------------------------------------------------------------------------
void vtkMath::Multiply3x3(const float A[3][3], const float v[3], float u[3])
{
  vtkMultiply3x3(A, v, u);
}

//------------------------------------------------------------------------------
void vtkMath::Multiply3x3(const double A[3][3], const double v[3], double u[3])
{
  vtkMultiply3x3(A, v, u);
}

//------------------------------------------------------------------------------
template <class T, class T2, class T3>
inline void vtkMultiplyMatrix3x3(const T A[3][3], const T2 B[3][3], T3 C[3][3])
{
  T3 D[3][3];

  for (int i = 0; i < 3; ++i)
  {
    D[0][i] = A[0][0] * B[0][i] + A[0][1] * B[1][i] + A[0][2] * B[2][i];
    D[1][i] = A[1][0] * B[0][i] + A[1][1] * B[1][i] + A[1][2] * B[2][i];
    D[2][i] = A[2][0] * B[0][i] + A[2][1] * B[1][i] + A[2][2] * B[2][i];
  }

  for (int j = 0; j < 3; ++j)
  {
    C[j][0] = D[j][0];
    C[j][1] = D[j][1];
    C[j][2] = D[j][2];
  }
}

//------------------------------------------------------------------------------
void vtkMath::Multiply3x3(const float A[3][3], const float B[3][3], float C[3][3])
{
  vtkMultiplyMatrix3x3(A, B, C);
}

//------------------------------------------------------------------------------
void vtkMath::Multiply3x3(const double A[3][3], const double B[3][3], double C[3][3])
{
  vtkMultiplyMatrix3x3(A, B, C);
}

//------------------------------------------------------------------------------
void vtkMath::MultiplyMatrix(const double* const* A, const double* const* B, unsigned int rowA,
  unsigned int colA, unsigned int rowB, unsigned int colB, double** C)
{
  // we need colA == rowB
  if (colA != rowB)
  {
    vtkGenericWarningMacro("Number of columns of A must match number of rows of B.");
  }

  // output matrix is rowA*colB

  // output row
  for (unsigned int i = 0; i < rowA; ++i)
  {
    // output col
    for (unsigned int j = 0; j < colB; ++j)
    {
      C[i][j] = 0;
      // sum for this point
      for (unsigned int k = 0; k < colA; ++k)
      {
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }
}

//------------------------------------------------------------------------------
template <class T1, class T2>
inline void vtkTranspose3x3(const T1 A[3][3], T2 AT[3][3])
{
  T2 tmp;
  tmp = A[1][0];
  AT[1][0] = A[0][1];
  AT[0][1] = tmp;
  tmp = A[2][0];
  AT[2][0] = A[0][2];
  AT[0][2] = tmp;
  tmp = A[2][1];
  AT[2][1] = A[1][2];
  AT[1][2] = tmp;

  AT[0][0] = A[0][0];
  AT[1][1] = A[1][1];
  AT[2][2] = A[2][2];
}

//------------------------------------------------------------------------------
void vtkMath::Transpose3x3(const float A[3][3], float AT[3][3])
{
  vtkTranspose3x3(A, AT);
}

//------------------------------------------------------------------------------
void vtkMath::Transpose3x3(const double A[3][3], double AT[3][3])
{
  vtkTranspose3x3(A, AT);
}

//------------------------------------------------------------------------------
template <class T1, class T2>
inline void vtkInvert3x3(const T1 A[3][3], T2 AI[3][3])
{
  double a1 = A[0][0];
  double b1 = A[0][1];
  double c1 = A[0][2];
  double a2 = A[1][0];
  double b2 = A[1][1];
  double c2 = A[1][2];
  double a3 = A[2][0];
  double b3 = A[2][1];
  double c3 = A[2][2];

  // Compute the adjoint
  double d1 = vtkMath::Determinant2x2(b2, b3, c2, c3);
  double d2 = -vtkMath::Determinant2x2(a2, a3, c2, c3);
  double d3 = vtkMath::Determinant2x2(a2, a3, b2, b3);

  double e1 = -vtkMath::Determinant2x2(b1, b3, c1, c3);
  double e2 = vtkMath::Determinant2x2(a1, a3, c1, c3);
  double e3 = -vtkMath::Determinant2x2(a1, a3, b1, b3);

  double f1 = vtkMath::Determinant2x2(b1, b2, c1, c2);
  double f2 = -vtkMath::Determinant2x2(a1, a2, c1, c2);
  double f3 = vtkMath::Determinant2x2(a1, a2, b1, b2);

  // Divide by the determinant
  double det = a1 * d1 + b1 * d2 + c1 * d3;

  AI[0][0] = d1 / det;
  AI[1][0] = d2 / det;
  AI[2][0] = d3 / det;

  AI[0][1] = e1 / det;
  AI[1][1] = e2 / det;
  AI[2][1] = e3 / det;

  AI[0][2] = f1 / det;
  AI[1][2] = f2 / det;
  AI[2][2] = f3 / det;
}

//------------------------------------------------------------------------------
void vtkMath::Invert3x3(const float A[3][3], float AI[3][3])
{
  vtkInvert3x3(A, AI);
}

//------------------------------------------------------------------------------
void vtkMath::Invert3x3(const double A[3][3], double AI[3][3])
{
  vtkInvert3x3(A, AI);
}

//------------------------------------------------------------------------------
template <class T>
inline void vtkIdentity3x3(T A[3][3])
{
  for (int i = 0; i < 3; ++i)
  {
    A[i][0] = A[i][1] = A[i][2] = T(0.0);
    A[i][i] = 1.0;
  }
}

//------------------------------------------------------------------------------
void vtkMath::Identity3x3(float A[3][3])
{
  vtkIdentity3x3(A);
}

//------------------------------------------------------------------------------
void vtkMath::Identity3x3(double A[3][3])
{
  vtkIdentity3x3(A);
}

//------------------------------------------------------------------------------
// Multiplying two quaternions
template <class T>
inline void vtkQuaternionMultiplication(const T q1[4], const T q2[4], T q[4])
{
  T ww = q1[0] * q2[0];
  T wx = q1[0] * q2[1];
  T wy = q1[0] * q2[2];
  T wz = q1[0] * q2[3];

  T xw = q1[1] * q2[0];
  T xx = q1[1] * q2[1];
  T xy = q1[1] * q2[2];
  T xz = q1[1] * q2[3];

  T yw = q1[2] * q2[0];
  T yx = q1[2] * q2[1];
  T yy = q1[2] * q2[2];
  T yz = q1[2] * q2[3];

  T zw = q1[3] * q2[0];
  T zx = q1[3] * q2[1];
  T zy = q1[3] * q2[2];
  T zz = q1[3] * q2[3];

  q[0] = ww - xx - yy - zz;
  q[1] = wx + xw + yz - zy;
  q[2] = wy - xz + yw + zx;
  q[3] = wz + xy - yx + zw;
}

//------------------------------------------------------------------------------
void vtkMath::MultiplyQuaternion(const float q1[4], const float q2[4], float q[4])
{
  vtkQuaternionMultiplication(q1, q2, q);
}

//------------------------------------------------------------------------------
void vtkMath::MultiplyQuaternion(const double q1[4], const double q2[4], double q[4])
{
  vtkQuaternionMultiplication(q1, q2, q);
}

//----------------------------------------------------------------------------
void vtkMath::RotateVectorByNormalizedQuaternion(const float v[3], const float q[4], float r[3])
{
  float f = std::sqrt(q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  float a[3];
  if (f != 0.0)
  {
    a[0] = q[1] / f;
    a[1] = q[2] / f;
    a[2] = q[3] / f;

    // atan2() provides a more accurate angle result than acos()
    float t = 2.0 * atan2(f, q[0]);

    float cosT = cos(t);
    float sinT = sin(t);
    float dotKV = a[0] * v[0] + a[1] * v[1] + a[2] * v[2];
    float crossKV[3];
    vtkMath::Cross(a, v, crossKV);

    r[0] = v[0] * cosT + crossKV[0] * sinT + a[0] * dotKV * (1.0 - cosT);
    r[1] = v[1] * cosT + crossKV[1] * sinT + a[1] * dotKV * (1.0 - cosT);
    r[2] = v[2] * cosT + crossKV[2] * sinT + a[2] * dotKV * (1.0 - cosT);
  }
  else
  {
    r[0] = v[0];
    r[1] = v[1];
    r[2] = v[2];
  }
}

void vtkMath::RotateVectorByNormalizedQuaternion(const double v[3], const double q[4], double r[3])
{
  double f = std::sqrt(q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
  double a[3];
  if (f != 0.0)
  {
    a[0] = q[1] / f;
    a[1] = q[2] / f;
    a[2] = q[3] / f;

    // atan2() provides a more accurate angle result than acos()
    double t = 2.0 * atan2(f, q[0]);

    double cosT = cos(t);
    double sinT = sin(t);
    double dotKV = a[0] * v[0] + a[1] * v[1] + a[2] * v[2];
    double crossKV[3];
    vtkMath::Cross(a, v, crossKV);

    r[0] = v[0] * cosT + crossKV[0] * sinT + a[0] * dotKV * (1.0 - cosT);
    r[1] = v[1] * cosT + crossKV[1] * sinT + a[1] * dotKV * (1.0 - cosT);
    r[2] = v[2] * cosT + crossKV[2] * sinT + a[2] * dotKV * (1.0 - cosT);
  }
  else
  {
    r[0] = v[0];
    r[1] = v[1];
    r[2] = v[2];
  }
}

void vtkMath::RotateVectorByWXYZ(const float v[3], const float q[4], float r[3])
{
  float cosT = cos(q[0]);
  float sinT = sin(q[0]);
  float dotKV = q[1] * v[0] + q[2] * v[1] + q[3] * v[2];
  float crossKV[3];
  vtkMath::Cross(&(q[1]), v, crossKV);

  r[0] = v[0] * cosT + crossKV[0] * sinT + q[1] * dotKV * (1.0 - cosT);
  r[1] = v[1] * cosT + crossKV[1] * sinT + q[2] * dotKV * (1.0 - cosT);
  r[2] = v[2] * cosT + crossKV[2] * sinT + q[3] * dotKV * (1.0 - cosT);
}

void vtkMath::RotateVectorByWXYZ(const double v[3], const double q[4], double r[3])
{
  double cosT = cos(q[0]);
  double sinT = sin(q[0]);
  double dotKV = q[1] * v[0] + q[2] * v[1] + q[3] * v[2];
  double crossKV[3];
  vtkMath::Cross(&(q[1]), v, crossKV);

  r[0] = v[0] * cosT + crossKV[0] * sinT + q[1] * dotKV * (1.0 - cosT);
  r[1] = v[1] * cosT + crossKV[1] * sinT + q[2] * dotKV * (1.0 - cosT);
  r[2] = v[2] * cosT + crossKV[2] * sinT + q[3] * dotKV * (1.0 - cosT);
}

//------------------------------------------------------------------------------
//  The orthogonalization is done via quaternions in order to avoid
//  having to use a singular value decomposition algorithm.
template <class T1, class T2>
inline void vtkOrthogonalize3x3(const T1 A[3][3], T2 B[3][3])
{
  int i;

  // copy the matrix
  for (i = 0; i < 3; ++i)
  {
    B[0][i] = A[0][i];
    B[1][i] = A[1][i];
    B[2][i] = A[2][i];
  }

  // Pivot the matrix to improve accuracy
  T2 scale[3];
  int index[3];
  T2 largest;

  // Loop over rows to get implicit scaling information
  for (i = 0; i < 3; ++i)
  {
    T2 x1 = std::abs(B[i][0]);
    T2 x2 = std::abs(B[i][1]);
    T2 x3 = std::abs(B[i][2]);
    largest = (x2 > x1 ? x2 : x1);
    largest = (x3 > largest ? x3 : largest);
    scale[i] = 1;
    if (largest != 0)
    {
      scale[i] /= largest;
    }
  }

  // first column
  T2 x1 = std::abs(B[0][0]) * scale[0];
  T2 x2 = std::abs(B[1][0]) * scale[1];
  T2 x3 = std::abs(B[2][0]) * scale[2];
  index[0] = 0;
  largest = x1;
  if (x2 >= largest)
  {
    largest = x2;
    index[0] = 1;
  }
  if (x3 >= largest)
  {
    index[0] = 2;
  }
  if (index[0] != 0)
  {
    vtkSwapVectors3(B[index[0]], B[0]);
    scale[index[0]] = scale[0];
  }

  // second column
  T2 y2 = std::abs(B[1][1]) * scale[1];
  T2 y3 = std::abs(B[2][1]) * scale[2];
  index[1] = 1;
  largest = y2;
  if (y3 >= largest)
  {
    index[1] = 2;
    vtkSwapVectors3(B[2], B[1]);
  }

  // third column
  index[2] = 2;

  // A quaternion can only describe a pure rotation, not
  // a rotation with a flip, therefore the flip must be
  // removed before the matrix is converted to a quaternion.
  bool flip = false;
  if (vtkDeterminant3x3(B) < 0)
  {
    flip = true;
    for (i = 0; i < 3; ++i)
    {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
    }
  }

  // Do orthogonalization using a quaternion intermediate
  // (this, essentially, does the orthogonalization via
  // diagonalization of an appropriately constructed symmetric
  // 4x4 matrix rather than by doing SVD of the 3x3 matrix)
  T2 quat[4];
  vtkMath::Matrix3x3ToQuaternion(B, quat);
  vtkMath::QuaternionToMatrix3x3(quat, B);

  // Put the flip back into the orthogonalized matrix.
  if (flip)
  {
    for (i = 0; i < 3; ++i)
    {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
    }
  }

  // Undo the pivoting
  if (index[1] != 1)
  {
    vtkSwapVectors3(B[index[1]], B[1]);
  }
  if (index[0] != 0)
  {
    vtkSwapVectors3(B[index[0]], B[0]);
  }
}

//------------------------------------------------------------------------------
void vtkMath::Orthogonalize3x3(const float A[3][3], float B[3][3])
{
  vtkOrthogonalize3x3(A, B);
}

//------------------------------------------------------------------------------
void vtkMath::Orthogonalize3x3(const double A[3][3], double B[3][3])
{
  vtkOrthogonalize3x3(A, B);
}

//------------------------------------------------------------------------------
float vtkMath::Norm(const float* x, int n)
{
  double sum = 0;
  for (int i = 0; i < n; ++i)
  {
    sum += x[i] * x[i];
  }

  return std::sqrt(sum);
}

//------------------------------------------------------------------------------
double vtkMath::Norm(const double* x, int n)
{
  double sum = 0;
  for (int i = 0; i < n; ++i)
  {
    sum += x[i] * x[i];
  }

  return std::sqrt(sum);
}

//------------------------------------------------------------------------------
bool vtkMath::ProjectVector(const float a[3], const float b[3], float projection[3])
{
  float bSquared = vtkMath::Dot(b, b);

  if (bSquared == 0)
  {
    projection[0] = 0;
    projection[1] = 0;
    projection[2] = 0;
    return false;
  }

  float scale = vtkMath::Dot(a, b) / bSquared;

  for (int i = 0; i < 3; ++i)
  {
    projection[i] = b[i];
  }
  vtkMath::MultiplyScalar(projection, scale);

  return true;
}

//------------------------------------------------------------------------------
bool vtkMath::ProjectVector(const double a[3], const double b[3], double projection[3])
{
  double bSquared = vtkMath::Dot(b, b);

  if (bSquared == 0)
  {
    projection[0] = 0;
    projection[1] = 0;
    projection[2] = 0;
    return false;
  }

  double scale = vtkMath::Dot(a, b) / bSquared;

  for (int i = 0; i < 3; ++i)
  {
    projection[i] = b[i] * scale;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkMath::ProjectVector2D(const float a[2], const float b[2], float projection[2])
{
  float bSquared = vtkMath::Dot2D(b, b);

  if (bSquared == 0)
  {
    projection[0] = 0;
    projection[1] = 0;
    return false;
  }

  float scale = vtkMath::Dot2D(a, b) / bSquared;

  for (int i = 0; i < 2; ++i)
  {
    projection[i] = b[i];
  }
  vtkMath::MultiplyScalar2D(projection, scale);

  return true;
}

//------------------------------------------------------------------------------
bool vtkMath::ProjectVector2D(const double a[2], const double b[2], double projection[2])
{
  double bSquared = vtkMath::Dot2D(b, b);

  if (bSquared == 0)
  {
    projection[0] = 0;
    projection[1] = 0;
    return false;
  }

  double scale = vtkMath::Dot2D(a, b) / bSquared;

  for (int i = 0; i < 2; ++i)
  {
    projection[i] = b[i];
  }
  vtkMath::MultiplyScalar2D(projection, scale);

  return true;
}

//------------------------------------------------------------------------------
// Extract the eigenvalues and eigenvectors from a 3x3 matrix.
// The eigenvectors (the columns of V) will be normalized.
// The eigenvectors are aligned optimally with the x, y, and z
// axes respectively.
template <class T1, class T2>
inline void vtkDiagonalize3x3(const T1 A[3][3], T2 w[3], T2 V[3][3])
{
  int i, j, k, maxI;
  T2 tmp, maxVal;

  // do the matrix[3][3] to **matrix conversion for Jacobi
  T2 C[3][3];
  T2 *ATemp[3], *VTemp[3];
  for (i = 0; i < 3; ++i)
  {
    C[i][0] = A[i][0];
    C[i][1] = A[i][1];
    C[i][2] = A[i][2];
    ATemp[i] = C[i];
    VTemp[i] = V[i];
  }

  // diagonalize using Jacobi
  vtkMath::JacobiN(ATemp, 3, w, VTemp);

  // if all the eigenvalues are the same, return identity matrix
  if (w[0] == w[1] && w[0] == w[2])
  {
    vtkMath::Identity3x3(V);
    return;
  }

  // transpose temporarily, it makes it easier to sort the eigenvectors
  vtkMath::Transpose3x3(V, V);

  // if two eigenvalues are the same, re-orthogonalize to optimally line
  // up the eigenvectors with the x, y, and z axes
  for (i = 0; i < 3; ++i)
  {
    if (w[(i + 1) % 3] == w[(i + 2) % 3]) // two eigenvalues are the same
    {
      // find maximum element of the independent eigenvector
      maxVal = std::abs(V[i][0]);
      maxI = 0;
      for (j = 1; j < 3; ++j)
      {
        if (maxVal < (tmp = std::abs(V[i][j])))
        {
          maxVal = tmp;
          maxI = j;
        }
      }
      // swap the eigenvector into its proper position
      if (maxI != i)
      {
        tmp = w[maxI];
        w[maxI] = w[i];
        w[i] = tmp;
        vtkSwapVectors3(V[i], V[maxI]);
      }
      // maximum element of eigenvector should be positive
      if (V[maxI][maxI] < 0)
      {
        V[maxI][0] = -V[maxI][0];
        V[maxI][1] = -V[maxI][1];
        V[maxI][2] = -V[maxI][2];
      }

      // re-orthogonalize the other two eigenvectors
      j = (maxI + 1) % 3;
      k = (maxI + 2) % 3;

      V[j][0] = 0.0;
      V[j][1] = 0.0;
      V[j][2] = 0.0;
      V[j][j] = 1.0;
      vtkMath::Cross(V[maxI], V[j], V[k]);
      vtkMath::Normalize(V[k]);
      vtkMath::Cross(V[k], V[maxI], V[j]);

      // transpose vectors back to columns
      vtkMath::Transpose3x3(V, V);
      return;
    }
  }

  // the three eigenvalues are different, just sort the eigenvectors
  // to align them with the x, y, and z axes

  // find the vector with the largest x element, make that vector
  // the first vector
  maxVal = std::abs(V[0][0]);
  maxI = 0;
  for (i = 1; i < 3; ++i)
  {
    if (maxVal < (tmp = std::abs(V[i][0])))
    {
      maxVal = tmp;
      maxI = i;
    }
  }
  // swap eigenvalue and eigenvector
  if (maxI != 0)
  {
    tmp = w[maxI];
    w[maxI] = w[0];
    w[0] = tmp;
    vtkSwapVectors3(V[maxI], V[0]);
  }
  // do the same for the y element
  if (std::abs(V[1][1]) < std::abs(V[2][1]))
  {
    tmp = w[2];
    w[2] = w[1];
    w[1] = tmp;
    vtkSwapVectors3(V[2], V[1]);
  }

  // ensure that the sign of the eigenvectors is correct
  for (i = 0; i < 2; ++i)
  {
    if (V[i][i] < 0)
    {
      V[i][0] = -V[i][0];
      V[i][1] = -V[i][1];
      V[i][2] = -V[i][2];
    }
  }
  // set sign of final eigenvector to ensure that determinant is positive
  if (vtkMath::Determinant3x3(V) < 0)
  {
    V[2][0] = -V[2][0];
    V[2][1] = -V[2][1];
    V[2][2] = -V[2][2];
  }

  // transpose the eigenvectors back again
  vtkMath::Transpose3x3(V, V);
}

//------------------------------------------------------------------------------
void vtkMath::Diagonalize3x3(const float A[3][3], float w[3], float V[3][3])
{
  vtkDiagonalize3x3(A, w, V);
}

//------------------------------------------------------------------------------
void vtkMath::Diagonalize3x3(const double A[3][3], double w[3], double V[3][3])
{
  vtkDiagonalize3x3(A, w, V);
}

//------------------------------------------------------------------------------
// Perform singular value decomposition on the matrix A:
//    A = U * W * VT
// where U and VT are orthogonal W is diagonal (the diagonal elements
// are returned in vector w).
// The matrices U and VT will both have positive determinants.
// The scale factors w are ordered according to how well the
// corresponding eigenvectors (in VT) match the x, y and z axes
// respectively.
//
// The singular value decomposition is used to decompose a linear
// transformation into a rotation, followed by a scale, followed
// by a second rotation.  The scale factors w will be negative if
// the determinant of matrix A is negative.
//
// Contributed by David Gobbi (dgobbi@irus.rri.on.ca)
template <class T1, class T2>
inline void vtkSingularValueDecomposition3x3(const T1 A[3][3], T2 U[3][3], T2 w[3], T2 VT[3][3])
{
  int i;
  T2 B[3][3];

  // copy so that A can be used for U or VT without risk
  for (i = 0; i < 3; ++i)
  {
    B[0][i] = A[0][i];
    B[1][i] = A[1][i];
    B[2][i] = A[2][i];
  }

  // temporarily flip if determinant is negative
  T2 d = vtkMath::Determinant3x3(B);
  if (d < 0)
  {
    for (i = 0; i < 3; ++i)
    {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
    }
  }

  // orthogonalize, diagonalize, etc.
  vtkMath::Orthogonalize3x3(B, U);
  vtkMath::Transpose3x3(B, B);
  vtkMath::Multiply3x3(B, U, VT);
  vtkMath::Diagonalize3x3(VT, w, VT);
  vtkMath::Multiply3x3(U, VT, U);
  vtkMath::Transpose3x3(VT, VT);

  // re-create the flip
  if (d < 0)
  {
    w[0] = -w[0];
    w[1] = -w[1];
    w[2] = -w[2];
  }

  /* paranoia check: recombine to ensure that the SVD is correct
  vtkMath::Transpose3x3(B, B);

  if (d < 0)
  {
    for (i = 0; i < 3; ++i)
    {
      B[0][i] = -B[0][i];
      B[1][i] = -B[1][i];
      B[2][i] = -B[2][i];
    }
  }

  int j;
  T2 maxerr = 0;
  T2 tmp;
  T2 M[3][3];
  T2 W[3][3];
  vtkMath::Identity3x3(W);
  W[0][0] = w[0]; W[1][1] = w[1]; W[2][2] = w[2];
  vtkMath::Identity3x3(M);
  vtkMath::Multiply3x3(M, U, M);
  vtkMath::Multiply3x3(M, W, M);
  vtkMath::Multiply3x3(M, VT, M);

  for (i = 0; i < 3; ++i)
  {
    for (j = 0; j < 3; ++j)
    {
      if ((tmp = std::abs(B[i][j] - M[i][j])) > maxerr)
      {
        maxerr = tmp;
      }
    }
  }

  vtkGenericWarningMacro("SingularValueDecomposition max error = " << maxerr);
  */
}

//------------------------------------------------------------------------------
void vtkMath::SingularValueDecomposition3x3(
  const float A[3][3], float U[3][3], float w[3], float VT[3][3])
{
  vtkSingularValueDecomposition3x3(A, U, w, VT);
}

//------------------------------------------------------------------------------
void vtkMath::SingularValueDecomposition3x3(
  const double A[3][3], double U[3][3], double w[3], double VT[3][3])
{
  vtkSingularValueDecomposition3x3(A, U, w, VT);
}

//------------------------------------------------------------------------------
void vtkMath::RGBToHSV(float r, float g, float b, float* h, float* s, float* v)
{
  double dh, ds, dv;
  vtkMath::RGBToHSV(r, g, b, &dh, &ds, &dv);
  *h = static_cast<float>(dh);
  *s = static_cast<float>(ds);
  *v = static_cast<float>(dv);
}

//------------------------------------------------------------------------------
void vtkMath::RGBToHSV(double r, double g, double b, double* h, double* s, double* v)
{
  constexpr double onethird = 1.0 / 3.0;
  constexpr double onesixth = 1.0 / 6.0;
  constexpr double twothird = 2.0 / 3.0;

  double cmax = r;
  double cmin = r;
  if (g > cmax)
  {
    cmax = g;
  }
  else if (g < cmin)
  {
    cmin = g;
  }
  if (b > cmax)
  {
    cmax = b;
  }
  else if (b < cmin)
  {
    cmin = b;
  }
  *v = cmax;

  if (*v > 0.0)
  {
    *s = (cmax - cmin) / cmax;
  }
  else
  {
    *s = 0.0;
  }
  if (*s > 0)
  {
    if (r == cmax)
    {
      *h = onesixth * (g - b) / (cmax - cmin);
    }
    else if (g == cmax)
    {
      *h = onethird + onesixth * (b - r) / (cmax - cmin);
    }
    else
    {
      *h = twothird + onesixth * (r - g) / (cmax - cmin);
    }
    if (*h < 0.0)
    {
      *h += 1.0;
    }
  }
  else
  {
    *h = 0.0;
  }
}

//------------------------------------------------------------------------------
void vtkMath::HSVToRGB(float h, float s, float v, float* r, float* g, float* b)
{
  double dr, dg, db;
  vtkMath::HSVToRGB(h, s, v, &dr, &dg, &db);
  *r = static_cast<float>(dr);
  *g = static_cast<float>(dg);
  *b = static_cast<float>(db);
}

//------------------------------------------------------------------------------
void vtkMath::HSVToRGB(double h, double s, double v, double* r, double* g, double* b)
{
  constexpr double onethird = 1.0 / 3.0;
  constexpr double onesixth = 1.0 / 6.0;
  constexpr double twothird = 2.0 / 3.0;
  constexpr double fivesixth = 5.0 / 6.0;

  // compute RGB from HSV
  if (h > onesixth && h <= onethird) // green/red
  {
    *g = 1.0;
    *r = (onethird - h) / onesixth;
    *b = 0.0;
  }
  else if (h > onethird && h <= 0.5) // green/blue
  {
    *g = 1.0;
    *b = (h - onethird) / onesixth;
    *r = 0.0;
  }
  else if (h > 0.5 && h <= twothird) // blue/green
  {
    *b = 1.0;
    *g = (twothird - h) / onesixth;
    *r = 0.0;
  }
  else if (h > twothird && h <= fivesixth) // blue/red
  {
    *b = 1.0;
    *r = (h - twothird) / onesixth;
    *g = 0.0;
  }
  else if (h > fivesixth && h <= 1.0) // red/blue
  {
    *r = 1.0;
    *b = (1.0 - h) / onesixth;
    *g = 0.0;
  }
  else // red/green
  {
    *r = 1.0;
    *g = h / onesixth;
    *b = 0.0;
  }

  // add Saturation to the equation.
  *r = (s * *r + (1.0 - s));
  *g = (s * *g + (1.0 - s));
  *b = (s * *b + (1.0 - s));

  *r *= v;
  *g *= v;
  *b *= v;
}

//------------------------------------------------------------------------------
void vtkMath::ProLabToXYZ(double L, double a, double b, double* x, double* y, double* z)
{
  // QI is the inverse of the Q transformation matrix having optimal parameters.
  double QI[4][4];
  QI[0][0] = 0.00137063282117354;
  QI[0][1] = 0.00138738203138321;
  QI[0][2] = 0.000816068851107095;
  QI[0][3] = 0;

  QI[1][0] = 0.00137063282117354;
  QI[1][1] = -0.000243154854293407;
  QI[1][2] = 0.000965329194924993;
  QI[1][3] = 0;

  QI[2][0] = 0.00137063282117354;
  QI[2][1] = 8.08345942991924e-05;
  QI[2][2] = -0.00317481896776885;
  QI[2][3] = 0;

  QI[3][0] = -0.00862936717882646;
  QI[3][1] = -0.000243154854293407;
  QI[3][2] = 0.000965329194924994;
  QI[3][3] = 1;

  double mProlab[4][4];
  mProlab[0][0] = L;
  mProlab[0][1] = a;
  mProlab[0][2] = b;
  mProlab[0][3] = 1;

  for (int i = 1; i < 4; ++i)
  {
    mProlab[i][0] = 0;
    mProlab[i][1] = 0;
    mProlab[i][2] = 0;
    mProlab[i][3] = 0;
  }

  double xyzHomog[4][4];
  MultiplyMatrix<4, 4, 4, vtkMatrixUtilities::Layout::Identity,
    vtkMatrixUtilities::Layout::Transpose>(mProlab, QI, xyzHomog);

  // Convert to XYZ
  double var_X =
    xyzHomog[0][0] / xyzHomog[0][3]; // ref_X = 0.9505  Observer= 2 deg, Illuminant= D65
  double var_Y = xyzHomog[0][1] / xyzHomog[0][3]; // ref_Y = 1.000
  double var_Z = xyzHomog[0][2] / xyzHomog[0][3]; // ref_Z = 1.089

  constexpr double ref_X = 0.9505;
  constexpr double ref_Y = 1.000;
  constexpr double ref_Z = 1.089;

  *x = var_X * ref_X;
  *y = var_Y * ref_Y;
  *z = var_Z * ref_Z;
}

//------------------------------------------------------------------------------
void vtkMath::XYZToProLab(double x, double y, double z, double* L, double* a, double* b)
{
  constexpr double ref_X = 0.9505;
  constexpr double ref_Y = 1.000;
  constexpr double ref_Z = 1.089;
  double var_X = x / ref_X; // ref_X = 0.9505  Observer= 2 deg, Illuminant= D65
  double var_Y = y / ref_Y; // ref_Y = 1.000
  double var_Z = z / ref_Z; // ref_Z = 1.089

  // Q transformation matrix having the optimal parameters.
  double Q[4][4];
  Q[0][0] = 75.54;
  Q[0][1] = 486.66;
  Q[0][2] = 167.39;
  Q[0][3] = 0;

  Q[1][0] = 617.72;
  Q[1][1] = -595.45;
  Q[1][2] = -22.27;
  Q[1][3] = 0;

  Q[2][0] = 48.34;
  Q[2][1] = 194.94;
  Q[2][2] = -243.28;
  Q[2][3] = 0;

  Q[3][0] = 0.7554;
  Q[3][1] = 3.8666;
  Q[3][2] = 1.6739;
  Q[3][3] = 1;

  // Construction of the matrix with the XYZ values for future multiplication.
  double mXYZ[4][4];
  mXYZ[0][0] = var_X;
  mXYZ[0][1] = var_Y;
  mXYZ[0][2] = var_Z;
  mXYZ[0][3] = 1;

  for (int i = 1; i < 4; ++i)
  {
    mXYZ[i][0] = 0;
    mXYZ[i][1] = 0;
    mXYZ[i][2] = 0;
    mXYZ[i][3] = 0;
  }

  double prolabHomog[4][4];

  MultiplyMatrix<4, 4, 4, vtkMatrixUtilities::Layout::Identity,
    vtkMatrixUtilities::Layout::Transpose>(mXYZ, Q, prolabHomog);

  // Convert to ProLab
  *L = prolabHomog[0][0] / prolabHomog[0][3];
  *a = prolabHomog[0][1] / prolabHomog[0][3];
  *b = prolabHomog[0][2] / prolabHomog[0][3];
}

//------------------------------------------------------------------------------
void vtkMath::LabToXYZ(double L, double a, double b, double* x, double* y, double* z)
{
  // LAB to XYZ
  double var_Y = (L + 16) / 116;
  double var_X = a / 500 + var_Y;
  double var_Z = var_Y - b / 200;

  if (pow(var_Y, 3) > 0.008856)
  {
    var_Y = pow(var_Y, 3);
  }
  else
  {
    var_Y = (var_Y - 16.0 / 116.0) / 7.787;
  }

  if (pow(var_X, 3) > 0.008856)
  {
    var_X = pow(var_X, 3);
  }
  else
  {
    var_X = (var_X - 16.0 / 116.0) / 7.787;
  }

  if (pow(var_Z, 3) > 0.008856)
  {
    var_Z = pow(var_Z, 3);
  }
  else
  {
    var_Z = (var_Z - 16.0 / 116.0) / 7.787;
  }
  constexpr double ref_X = 0.9505;
  constexpr double ref_Y = 1.000;
  constexpr double ref_Z = 1.089;
  *x = ref_X * var_X; // ref_X = 0.9505  Observer= 2 deg Illuminant= D65
  *y = ref_Y * var_Y; // ref_Y = 1.000
  *z = ref_Z * var_Z; // ref_Z = 1.089
}

//------------------------------------------------------------------------------
void vtkMath::XYZToLab(double x, double y, double z, double* L, double* a, double* b)
{
  constexpr double ref_X = 0.9505;
  constexpr double ref_Y = 1.000;
  constexpr double ref_Z = 1.089;
  double var_X = x / ref_X; // ref_X = 0.9505  Observer= 2 deg, Illuminant= D65
  double var_Y = y / ref_Y; // ref_Y = 1.000
  double var_Z = z / ref_Z; // ref_Z = 1.089

  if (var_X > 0.008856)
  {
    var_X = pow(var_X, 1.0 / 3.0);
  }
  else
  {
    var_X = (7.787 * var_X) + (16.0 / 116.0);
  }
  if (var_Y > 0.008856)
  {
    var_Y = pow(var_Y, 1.0 / 3.0);
  }
  else
  {
    var_Y = (7.787 * var_Y) + (16.0 / 116.0);
  }
  if (var_Z > 0.008856)
  {
    var_Z = pow(var_Z, 1.0 / 3.0);
  }
  else
  {
    var_Z = (7.787 * var_Z) + (16.0 / 116.0);
  }

  *L = (116 * var_Y) - 16;
  *a = 500 * (var_X - var_Y);
  *b = 200 * (var_Y - var_Z);
}

//------------------------------------------------------------------------------
void vtkMath::XYZToRGB(double x, double y, double z, double* r, double* g, double* b)
{
  // double ref_X = 0.9505;        //Observer = 2 deg Illuminant = D65
  // double ref_Y = 1.000;
  // double ref_Z = 1.089;

  *r = x * 3.2406 + y * -1.5372 + z * -0.4986;
  *g = x * -0.9689 + y * 1.8758 + z * 0.0415;
  *b = x * 0.0557 + y * -0.2040 + z * 1.0570;

  // The following performs a "gamma correction" specified by the sRGB color
  // space.  sRGB is defined by a canonical definition of a display monitor and
  // has been standardized by the International Electrotechnical Commission (IEC
  // 61966-2-1).  The nonlinearity of the correction is designed to make the
  // colors more perceptually uniform.  This color space has been adopted by
  // several applications including Adobe Photoshop and Microsoft Windows color
  // management.  OpenGL is agnostic on its RGB color space, but it is reasonable
  // to assume it is close to this one.
  if (*r > 0.0031308)
  {
    *r = 1.055 * (pow(*r, (1 / 2.4))) - 0.055;
  }
  else
  {
    *r = 12.92 * (*r);
  }
  if (*g > 0.0031308)
  {
    *g = 1.055 * (pow(*g, (1 / 2.4))) - 0.055;
  }
  else
  {
    *g = 12.92 * (*g);
  }
  if (*b > 0.0031308)
  {
    *b = 1.055 * (pow(*b, (1 / 2.4))) - 0.055;
  }
  else
  {
    *b = 12.92 * (*b);
  }

  // Clip colors. ideally we would do something that is perceptually closest
  // (since we can see colors outside of the display gamut), but this seems to
  // work well enough.
  double maxVal = *r;
  maxVal = std::max(maxVal, *g);
  maxVal = std::max(maxVal, *b);
  if (maxVal > 1.0)
  {
    *r /= maxVal;
    *g /= maxVal;
    *b /= maxVal;
  }
  *r = std::max<double>(*r, 0);
  *g = std::max<double>(*g, 0);
  *b = std::max<double>(*b, 0);
}

//------------------------------------------------------------------------------
void vtkMath::RGBToXYZ(double r, double g, double b, double* x, double* y, double* z)
{
  // The following performs a "gamma correction" specified by the sRGB color
  // space.  sRGB is defined by a canonical definition of a display monitor and
  // has been standardized by the International Electrotechnical Commission (IEC
  // 61966-2-1).  The nonlinearity of the correction is designed to make the
  // colors more perceptually uniform.  This color space has been adopted by
  // several applications including Adobe Photoshop and Microsoft Windows color
  // management.  OpenGL is agnostic on its RGB color space, but it is reasonable
  // to assume it is close to this one.
  if (r > 0.04045)
  {
    r = pow((r + 0.055) / 1.055, 2.4);
  }
  else
  {
    r = r / 12.92;
  }
  if (g > 0.04045)
  {
    g = pow((g + 0.055) / 1.055, 2.4);
  }
  else
  {
    g = g / 12.92;
  }
  if (b > 0.04045)
  {
    b = pow((b + 0.055) / 1.055, 2.4);
  }
  else
  {
    b = b / 12.92;
  }

  // Observer. = 2 deg, Illuminant = D65
  *x = r * 0.4124 + g * 0.3576 + b * 0.1805;
  *y = r * 0.2126 + g * 0.7152 + b * 0.0722;
  *z = r * 0.0193 + g * 0.1192 + b * 0.9505;
}

//------------------------------------------------------------------------------
void vtkMath::RGBToProLab(double red, double green, double blue, double* L, double* a, double* b)
{
  double x, y, z;
  vtkMath::RGBToXYZ(red, green, blue, &x, &y, &z);
  vtkMath::XYZToProLab(x, y, z, L, a, b);
}

//------------------------------------------------------------------------------
void vtkMath::ProLabToRGB(double L, double a, double b, double* red, double* green, double* blue)
{
  double x, y, z;
  vtkMath::ProLabToXYZ(L, a, b, &x, &y, &z);
  vtkMath::XYZToRGB(x, y, z, red, green, blue);
}

//------------------------------------------------------------------------------
void vtkMath::RGBToLab(double red, double green, double blue, double* L, double* a, double* b)
{
  double x, y, z;
  vtkMath::RGBToXYZ(red, green, blue, &x, &y, &z);
  vtkMath::XYZToLab(x, y, z, L, a, b);
}

//------------------------------------------------------------------------------
void vtkMath::LabToRGB(double L, double a, double b, double* red, double* green, double* blue)
{
  double x, y, z;
  vtkMath::LabToXYZ(L, a, b, &x, &y, &z);
  vtkMath::XYZToRGB(x, y, z, red, green, blue);
}

//------------------------------------------------------------------------------
void vtkMath::ClampValues(double* values, int nb_values, const double range[2])
{
  if (!values || nb_values <= 0 || !range)
  {
    return;
  }

  const double* values_end = values + nb_values;
  while (values < values_end)
  {
    *values = vtkMath::ClampValue(*values, range[0], range[1]);
    ++values;
  }
}

//------------------------------------------------------------------------------
void vtkMath::ClampValues(
  const double* values, int nb_values, const double range[2], double* clamped_values)
{
  if (!values || nb_values <= 0 || !range || !clamped_values)
  {
    return;
  }

  const double* values_end = values + nb_values;
  while (values < values_end)
  {
    *clamped_values = vtkMath::ClampValue(*values, range[0], range[1]);
    ++values;
    ++clamped_values;
  }
}

//------------------------------------------------------------------------------
int vtkMath::GetScalarTypeFittingRange(
  double range_min, double range_max, double scale, double shift)
{
  class TypeRange
  {
  public:
    int Type;
    double Min;
    double Max;
  };

  constexpr TypeRange FloatTypes[] = { { VTK_FLOAT, VTK_FLOAT_MIN, VTK_FLOAT_MAX },
    { VTK_DOUBLE, VTK_DOUBLE_MIN, VTK_DOUBLE_MAX } };

  constexpr TypeRange IntTypes[] = { { VTK_BIT, VTK_BIT_MIN, VTK_BIT_MAX },
    { VTK_CHAR, VTK_CHAR_MIN, VTK_CHAR_MAX },
    { VTK_SIGNED_CHAR, VTK_SIGNED_CHAR_MIN, VTK_SIGNED_CHAR_MAX },
    { VTK_UNSIGNED_CHAR, VTK_UNSIGNED_CHAR_MIN, VTK_UNSIGNED_CHAR_MAX },
    { VTK_SHORT, VTK_SHORT_MIN, VTK_SHORT_MAX },
    { VTK_UNSIGNED_SHORT, VTK_UNSIGNED_SHORT_MIN, VTK_UNSIGNED_SHORT_MAX },
    { VTK_INT, VTK_INT_MIN, VTK_INT_MAX },
    { VTK_UNSIGNED_INT, VTK_UNSIGNED_INT_MIN, VTK_UNSIGNED_INT_MAX },
    { VTK_LONG, static_cast<double>(VTK_LONG_MIN), static_cast<double>(VTK_LONG_MAX) },
    { VTK_UNSIGNED_LONG, static_cast<double>(VTK_UNSIGNED_LONG_MIN),
      static_cast<double>(VTK_UNSIGNED_LONG_MAX) },
    { VTK_LONG_LONG, static_cast<double>(VTK_LONG_LONG_MIN),
      static_cast<double>(VTK_LONG_LONG_MAX) },
    { VTK_UNSIGNED_LONG_LONG, static_cast<double>(VTK_UNSIGNED_LONG_LONG_MIN),
      static_cast<double>(VTK_UNSIGNED_LONG_LONG_MAX) } };

  // If the range, scale or shift are decimal number, just browse
  // the decimal types

  double intpart;

  int range_min_is_int = (modf(range_min, &intpart) == 0.0);
  int range_max_is_int = (modf(range_max, &intpart) == 0.0);
  int scale_is_int = (modf(scale, &intpart) == 0.0);
  int shift_is_int = (modf(shift, &intpart) == 0.0);

  range_min = range_min * scale + shift;
  range_max = range_max * scale + shift;

  if (range_min_is_int && range_max_is_int && scale_is_int && shift_is_int)
  {
    for (unsigned int i = 0; i < sizeof(IntTypes) / sizeof(TypeRange); ++i)
    {
      if (IntTypes[i].Min <= range_min && range_max <= IntTypes[i].Max)
      {
        return IntTypes[i].Type;
      }
    }
  }

  for (unsigned int i = 0; i < sizeof(FloatTypes) / sizeof(TypeRange); ++i)
  {
    if (FloatTypes[i].Min <= range_min && range_max <= FloatTypes[i].Max)
    {
      return FloatTypes[i].Type;
    }
  }

  return -1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::GetAdjustedScalarRange(vtkDataArray* array, int comp, double range[2])
{
  if (!array || comp < 0 || comp >= array->GetNumberOfComponents())
  {
    return 0;
  }

  array->GetRange(range, comp);

  switch (array->GetDataType())
  {
    case VTK_UNSIGNED_CHAR:
      range[0] = array->GetDataTypeMin();
      range[1] = array->GetDataTypeMax();
      break;

    case VTK_UNSIGNED_SHORT:
      range[0] = array->GetDataTypeMin();
      if (range[1] <= 4095.0)
      {
        if (range[1] > VTK_UNSIGNED_CHAR_MAX)
        {
          range[1] = 4095.0;
        }
      }
      else
      {
        range[1] = array->GetDataTypeMax();
      }
      break;
    default:
      assert("check: impossible case." && 0); // reaching this line is a bug.
      break;
  }

  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::ExtentIsWithinOtherExtent(const int extent1[6], const int extent2[6])
{
  if (!extent1 || !extent2)
  {
    return 0;
  }

  for (int i = 0; i < 6; i += 2)
  {
    if (extent1[i] < extent2[i] || extent1[i] > extent2[i + 1] || extent1[i + 1] < extent2[i] ||
      extent1[i + 1] > extent2[i + 1])
    {
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------

vtkTypeBool vtkMath::BoundsIsWithinOtherBounds(
  const double bounds1[6], const double bounds2[6], const double delta[3])
{
  if (!bounds1 || !bounds2)
  {
    return 0;
  }
  for (int i = 0; i < 6; i += 2)
  {
    if (bounds1[i] + delta[i / 2] < bounds2[i] || bounds1[i] - delta[i / 2] > bounds2[i + 1] ||
      bounds1[i + 1] + delta[i / 2] < bounds2[i] || bounds1[i + 1] - delta[i / 2] > bounds2[i + 1])
    {
      return 0;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
vtkTypeBool vtkMath::PointIsWithinBounds(
  const double point[3], const double bounds[6], const double delta[3])
{
  if (!point || !bounds || !delta)
  {
    return 0;
  }
  return /*i = 0*/ point[0] + delta[0] >= bounds[0] && point[0] - delta[0] <= bounds[1] &&
    /*i = 1*/ point[1] + delta[1] >= bounds[2] && point[1] - delta[1] <= bounds[3] &&
    /*i = 2*/ point[2] + delta[2] >= bounds[4] && point[2] - delta[2] <= bounds[5];
}

//------------------------------------------------------------------------------
int vtkMath::PlaneIntersectsAABB(
  const double bounds[6], const double normal[3], const double point[3])
{
  if (!bounds || !point || !normal)
  {
    return -2;
  }

  double nPoint[3];
  double pPoint[3];

  // X Component
  if (normal[0] >= 0)
  {
    nPoint[0] = bounds[0];
    pPoint[0] = bounds[1];
  }
  else
  {
    nPoint[0] = bounds[1];
    pPoint[0] = bounds[0];
  }

  // Y Component
  if (normal[1] >= 0)
  {
    nPoint[1] = bounds[2];
    pPoint[1] = bounds[3];
  }
  else
  {
    nPoint[1] = bounds[3];
    pPoint[1] = bounds[2];
  }

  // Z Component
  if (normal[2] >= 0)
  {
    nPoint[2] = bounds[4];
    pPoint[2] = bounds[5];
  }
  else
  {
    nPoint[2] = bounds[5];
    pPoint[2] = bounds[4];
  }

  // Compute distances from nPoint/pPoint to the plane
  // Distance = unit_N  *  (P_x - P_plane)
  //          = a * px_1 + b * px_2 + c * px_3 - d
  double const d = vtkMath::Dot(normal, point);

  if ((nPoint[0] * normal[0] + nPoint[1] * normal[1] + nPoint[2] * normal[2] - d) > 0)
  {
    return 1;
  }
  else if ((pPoint[0] * normal[0] + pPoint[1] * normal[1] + pPoint[2] * normal[2] - d) < 0)
  {
    return -1;
  }

  return 0;
}

//------------------------------------------------------------------------------
double vtkMath::AngleBetweenVectors(const double v1[3], const double v2[3])
{
  double cross[3];
  vtkMath::Cross(v1, v2, cross);
  return atan2(vtkMath::Norm(cross), vtkMath::Dot(v1, v2));
}

//------------------------------------------------------------------------------
double vtkMath::SignedAngleBetweenVectors(
  const double v1[3], const double v2[3], const double vn[3])
{
  double cross[3];
  vtkMath::Cross(v1, v2, cross);
  double angle = atan2(vtkMath::Norm(cross), vtkMath::Dot(v1, v2));
  return vtkMath::Dot(cross, vn) >= 0 ? angle : -angle;
}

//------------------------------------------------------------------------------
double vtkMath::GaussianAmplitude(double variance, double distanceFromMean)
{
  return 1. / (std::sqrt(2. * vtkMath::Pi() * variance)) *
    exp(-(pow(distanceFromMean, 2)) / (2. * variance));
}

//------------------------------------------------------------------------------
double vtkMath::GaussianAmplitude(double mean, double variance, double position)
{
  double distanceToMean = std::abs(mean - position);
  return GaussianAmplitude(variance, distanceToMean);
}

//------------------------------------------------------------------------------
double vtkMath::GaussianWeight(double variance, double distanceFromMean)
{
  return exp(-(pow(distanceFromMean, 2)) / (2. * variance));
}

//------------------------------------------------------------------------------
double vtkMath::GaussianWeight(double mean, double variance, double position)
{
  double distanceToMean = std::abs(mean - position);
  return GaussianWeight(variance, distanceToMean);
}

//------------------------------------------------------------------------------
double vtkMath::Solve3PointCircle(
  const double p1[3], const double p2[3], const double p3[3], double center[3])
{
  double v21[3], v32[3], v13[3];
  double v12[3], v23[3], v31[3];
  for (int i = 0; i < 3; ++i)
  {
    v21[i] = p1[i] - p2[i];
    v32[i] = p2[i] - p3[i];
    v13[i] = p3[i] - p1[i];
    v12[i] = -v21[i];
    v23[i] = -v32[i];
    v31[i] = -v13[i];
  }

  double norm12 = vtkMath::Norm(v12);
  double norm23 = vtkMath::Norm(v23);
  double norm13 = vtkMath::Norm(v13);

  double crossv21v32[3];
  vtkMath::Cross(v21, v32, crossv21v32);
  double normCross = vtkMath::Norm(crossv21v32);

  double radius = (norm12 * norm23 * norm13) / (2. * normCross);

  double alpha = ((norm23 * norm23) * vtkMath::Dot(v21, v31)) / (2. * normCross * normCross);
  double beta = ((norm13 * norm13) * vtkMath::Dot(v12, v32)) / (2. * normCross * normCross);
  double gamma = ((norm12 * norm12) * vtkMath::Dot(v13, v23)) / (2. * normCross * normCross);

  for (int i = 0; i < 3; ++i)
  {
    center[i] = alpha * p1[i] + beta * p2[i] + gamma * p3[i];
  }
  return radius;
}

//------------------------------------------------------------------------------
void vtkMath::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Seed: " << vtkMath::Internal->Uniform->GetSeed() << "\n";
}

//------------------------------------------------------------------------------
double vtkMath::Inf()
{
  return std::numeric_limits<double>::infinity();
}

//------------------------------------------------------------------------------
double vtkMath::NegInf()
{
  return -std::numeric_limits<double>::infinity();
}

//------------------------------------------------------------------------------
double vtkMath::Nan()
{
  return std::numeric_limits<double>::quiet_NaN();
}

//------------------------------------------------------------------------------
#ifndef VTK_MATH_ISINF_IS_INLINE
vtkTypeBool vtkMath::IsInf(double x)
{
  return (!vtkMath::IsNan(x) && !((x < vtkMath::Inf()) && (x > vtkMath::NegInf())));
}
#endif

//------------------------------------------------------------------------------
#ifndef VTK_MATH_ISNAN_IS_INLINE
vtkTypeBool vtkMath::IsNan(double x)
{
  return !((x <= 0.0) || (x >= 0.0));
}
#endif

//------------------------------------------------------------------------------
#ifndef VTK_MATH_ISFINITE_IS_INLINE
bool vtkMath::IsFinite(double x)
{
  return !vtkMath::IsNan(x) && !vtkMath::IsInf(x);
}
#endif

//------------------------------------------------------------------------------
int vtkMath::QuadraticRoot(double a, double b, double c, double min, double max, double* u)
{
  if (a == 0.0) // then its close to 0
  {
    if (b != 0.0) // not close to 0
    {
      u[0] = -c / b;
      if (u[0] > min && u[0] < max) // its in the interval
      {
        return 1; // 1 soln found
      }
      else // its not in the interval
      {
        return 0;
      }
    }
    else
    {
      return 0;
    }
  }
  double d = b * b - 4 * a * c; // discriminant
  if (d <= 0.0)                 // single or no root
  {
    if (d == 0.0) // close to 0
    {
      u[0] = -b / a;
      if (u[0] > min && u[0] < max) // its in the interval
      {
        return 1;
      }
      else // its not in the interval
      {
        return 0;
      }
    }
    else // no root d must be below 0
    {
      return 0;
    }
  }
  double q = -0.5 * (b + copysign(sqrt(d), b));
  u[0] = c / q;
  u[1] = q / a;

  if ((u[0] > min && u[0] < max) && (u[1] > min && u[1] < max))
  {
    return 2;
  }
  else if (u[0] > min && u[0] < max) // then one wasn't in interval
  {
    return 1;
  }
  else if (u[1] > min && u[1] < max)
  { // make it easier, make u[0] be the valid one always
    std::swap(u[0], u[1]);
    return 1;
  }
  return 0;
}
VTK_ABI_NAMESPACE_END
