/*
 * Tests for shapeArabic.
 *
 * Arabic characters and their key codepoints:
 *   Alef  = 0x0627  (non-connectable: only isolated/final)
 *   Baa   = 0x0628  (dual-connecting: has all 4 forms)
 *   Taa   = 0x062A  (dual-connecting)
 *   Lam   = 0x0644  (dual-connecting, ligature base)
 *   Meem  = 0x0645  (dual-connecting)
 *   Noon  = 0x0646  (dual-connecting)
 *
 * Presentation form references (from Unifont/UnicodeArabicPresentation):
 *   Baa:  Isolated=0xFE8F, Final=0xFE90, Initial=0xFE91, Medial=0xFE92
 *   Alef: Isolated=0xFE8D, Final=0xFE8E (no initial/medial — non-connecting)
 *   Lam:  Isolated=0xFEDD, Final=0xFEDE, Initial=0xFEDF, Medial=0xFEE0
 *   Meem: Isolated=0xFEE1, Final=0xFEE2, Initial=0xFEE3, Medial=0xFEE4
 *   Noon: Isolated=0xFEE5, Final=0xFEE6, Initial=0xFEE7, Medial=0xFEE8
 */

#include "test_harness.hpp"
#include "ArabicShaping.hpp"
#include "UnicodeArabicPresentation.hpp"

using namespace focus;

using namespace UnicodeArabicPresentation;

TEST(arabic_isolated_single_char) {
    // Single Baa (no neighbors) → isolated form
    UNICODE_CODEPOINT cps[] = {0x0628};
    shapeArabic(cps, 1);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Isolated));
}

TEST(arabic_two_connecting_chars) {
    // Baa + Baa → first gets initial form, second gets final form
    UNICODE_CODEPOINT cps[] = {0x0628, 0x0628};
    shapeArabic(cps, 2);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Initial));
    ASSERT_EQ(cps[1], getForm(0x0628, Form::Final));
}

TEST(arabic_three_connecting_chars) {
    // Baa + Meem + Noon → initial, medial, final
    UNICODE_CODEPOINT cps[] = {0x0628, 0x0645, 0x0646};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Initial));
    ASSERT_EQ(cps[1], getForm(0x0645, Form::Medial));
    ASSERT_EQ(cps[2], getForm(0x0646, Form::Final));
}

TEST(arabic_alef_breaks_medial) {
    // Baa + Alef + Baa. Alef is non-connecting (no initial/medial form).
    // Baa(initial) + Alef(final) + Baa(isolated)
    // Actually: Baa connects forward to Alef, so Baa=initial.
    // Alef can connect backward (has final form), so Alef=final.
    // After Alef, connectivity breaks because Alef doesn't connect forward.
    // Baa=isolated.
    UNICODE_CODEPOINT cps[] = {0x0628, 0x0627, 0x0628};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Initial));
    ASSERT_EQ(cps[1], getForm(0x0627, Form::Final));
    ASSERT_EQ(cps[2], getForm(0x0628, Form::Isolated));
}

TEST(arabic_lam_alef_ligature_isolated) {
    // Lam + Alef (no previous connector) → ligature isolated form
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0627};
    shapeArabic(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFEFB);  // Lam-Alef isolated
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x200B);  // Alef consumed (zero width space)
}

TEST(arabic_lam_alef_ligature_final) {
    // Baa + Lam + Alef → Baa initial, Lam-Alef final form
    UNICODE_CODEPOINT cps[] = {0x0628, 0x0644, 0x0627};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Initial));
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0xFEFC);  // Lam-Alef final
    ASSERT_EQ(cps[2], (UNICODE_CODEPOINT)0x200B);  // Alef consumed
}

TEST(arabic_lam_alef_madda_ligature) {
    // Lam + Alef-Madda (U+0622)
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0622};
    shapeArabic(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFEF5);  // Lam-Alef-Madda isolated
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x200B);
}

TEST(arabic_lam_alef_hamza_above_ligature) {
    // Lam + Alef-Hamza-Above (U+0623)
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0623};
    shapeArabic(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFEF7);  // Lam-Alef-Hamza-Above isolated
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x200B);
}

TEST(arabic_lam_alef_hamza_below_ligature) {
    // Lam + Alef-Hamza-Below (U+0625)
    UNICODE_CODEPOINT cps[] = {0x0644, 0x0625};
    shapeArabic(cps, 2);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)0xFEF9);  // Lam-Alef-Hamza-Below isolated
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x200B);
}

TEST(arabic_nsm_doesnt_break_connectivity) {
    // Baa + Fathah(NSM, U+064E) + Baa → fathah shouldn't break connectivity
    // Baa should be initial, second Baa should be final
    UNICODE_CODEPOINT cps[] = {0x0628, 0x064E, 0x0628};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Initial));
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x064E);  // NSM unchanged
    ASSERT_EQ(cps[2], getForm(0x0628, Form::Final));
}

TEST(arabic_numbers_break_connectivity) {
    // Baa + Arabic-Indic Digit 1 (U+0661) + Baa
    // Numbers are in the Arabic block but not shapeable → break connectivity
    UNICODE_CODEPOINT cps[] = {0x0628, 0x0661, 0x0628};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Isolated));
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)0x0661);  // unchanged
    ASSERT_EQ(cps[2], getForm(0x0628, Form::Isolated));
}

TEST(arabic_latin_breaks_connectivity) {
    // Baa + 'A' + Baa → Latin 'A' is not in Arabic block
    UNICODE_CODEPOINT cps[] = {0x0628, 'A', 0x0628};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], getForm(0x0628, Form::Isolated));
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)'A');  // unchanged
    ASSERT_EQ(cps[2], getForm(0x0628, Form::Isolated));
}

TEST(arabic_mixed_text_only_shapes_arabic) {
    // "Hi" + Baa → Latin unchanged, Arabic shaped
    UNICODE_CODEPOINT cps[] = {'H', 'i', 0x0628};
    shapeArabic(cps, 3);
    ASSERT_EQ(cps[0], (UNICODE_CODEPOINT)'H');
    ASSERT_EQ(cps[1], (UNICODE_CODEPOINT)'i');
    ASSERT_EQ(cps[2], getForm(0x0628, Form::Isolated));
}
