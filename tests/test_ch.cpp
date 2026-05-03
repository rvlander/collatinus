#include "test_utils.h"
#include "ch.h"

int main()
{
    // --- lowercase diacritics (all stripped) ---
    CHECK(Ch::atone("\xC4\x81") == "a", "ā → a");
    CHECK(Ch::atone("\xC4\x83") == "a", "ă → a");
    CHECK(Ch::atone("\xC4\x93") == "e", "ē → e");
    CHECK(Ch::atone("\xC4\x95") == "e", "ĕ → e");
    CHECK(Ch::atone("\xC4\xAB") == "i", "ī → i");
    CHECK(Ch::atone("\xC4\xAD") == "i", "ĭ → i");
    CHECK(Ch::atone("\xC5\x8D") == "o", "ō → o");
    CHECK(Ch::atone("\xC5\x8F") == "o", "ŏ → o");
    CHECK(Ch::atone("\xC5\xAB") == "u", "ū → u");
    CHECK(Ch::atone("\xC5\xAD") == "u", "ŭ → u");
    CHECK(Ch::atone("\xC8\xB3") == "y", "ȳ (U+0233) → y");
    CHECK(Ch::atone("\xD1\x9E") == "y", "ў (U+045E) → y");

    // --- uppercase diacritics stripped when bdc=false (default) ---
    CHECK(Ch::atone("\xC4\x80") == "A", "Ā → A");
    CHECK(Ch::atone("\xC4\x82") == "A", "Ă → A");
    CHECK(Ch::atone("\xC4\x92") == "E", "Ē → E");
    CHECK(Ch::atone("\xC4\x94") == "E", "Ĕ → E");
    CHECK(Ch::atone("\xC4\xAA") == "I", "Ī → I");
    CHECK(Ch::atone("\xC4\xAC") == "I", "Ĭ → I");
    CHECK(Ch::atone("\xC5\x8C") == "O", "Ō → O");
    CHECK(Ch::atone("\xC5\x8E") == "O", "Ŏ → O");
    CHECK(Ch::atone("\xC5\xAA") == "U", "Ū → U");
    CHECK(Ch::atone("\xC5\xAC") == "U", "Ŭ → U");
    CHECK(Ch::atone("\xC8\xB2") == "Y", "Ȳ (U+0232) → Y");
    CHECK(Ch::atone("\xD0\x8E") == "Y", "Ў (U+040E) → Y");

    // --- bdc=true: uppercase diacritics kept unchanged ---
    CHECK(Ch::atone("\xC4\x80", true) == "\xC4\x80", "Ā kept when bdc=true");
    CHECK(Ch::atone("\xC4\x92", true) == "\xC4\x92", "Ē kept when bdc=true");
    CHECK(Ch::atone("\xC4\xAA", true) == "\xC4\xAA", "Ī kept when bdc=true");
    CHECK(Ch::atone("\xC5\x8C", true) == "\xC5\x8C", "Ō kept when bdc=true");
    CHECK(Ch::atone("\xC5\xAA", true) == "\xC5\xAA", "Ū kept when bdc=true");
    CHECK(Ch::atone("\xC8\xB2", true) == "\xC8\xB2", "Ȳ kept when bdc=true");

    // --- lowercase still stripped even when bdc=true ---
    CHECK(Ch::atone("\xC4\x81", true) == "a", "ā stripped even when bdc=true");

    // --- special characters ---
    CHECK(Ch::atone("\xC4\xB1") == "i", "ı (U+0131 dotless i) → i");
    CHECK(Ch::atone("\xE1\xBB\xA5") == "u", "ụ (U+1EE5) → u");

    // --- combining breve stripped ---
    // ā + combining breve = \xC4\x81\xCC\x86 → "a" (breve dropped after atone strips ā)
    CHECK(Ch::atone("\xC4\x81\xCC\x86") == "a", "ā + combining breve → a");
    // plain vowel + combining breve → vowel only (breve dropped)
    CHECK(Ch::atone("a\xCC\x86") == "a", "a + combining breve → a");

    // --- edge cases ---
    CHECK(Ch::atone("") == "", "empty string");
    CHECK(Ch::atone("amicus") == "amicus", "plain ASCII unchanged");

    // --- realistic mixed word ---
    // āmīcus = \xC4\x81 m \xC4\xAB cus
    CHECK(Ch::atone("\xC4\x81m\xC4\xAB" "cus") == "amicus", "āmīcus → amicus");
    // Rōma = R \xC5\x8D ma
    CHECK(Ch::atone("R\xC5\x8Dma") == "Roma", "Rōma → Roma");

    RUN_TESTS();
}
