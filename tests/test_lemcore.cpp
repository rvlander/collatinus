#include "test_utils.h"
#include "lemCore.h"
#include "lemme.h"

// Resolve the path to bin/data/ relative to the tests/ dir
static const std::string RES = "../bin/data/";

int main()
{
    // --- Test 1: default cible "fr en es" loads fr, en, es ---
    {
        LemCore lc(RES, "fr en es");
        CHECK(lc.cibles().count("fr") == 1, "default cible: fr loaded");
        CHECK(lc.cibles().count("en") == 1, "default cible: en loaded");
        CHECK(lc.cibles().count("es") == 1, "default cible: es loaded");
        CHECK(lc.cibles().count("de") == 0, "default cible: de NOT loaded");
        CHECK(lc.cibles().count("it") == 0, "default cible: it NOT loaded");
    }

    // --- Test 2: cible "fr" loads only fr + en (en is hardcoded fallback) ---
    {
        LemCore lc(RES, "fr");
        CHECK(lc.cibles().count("fr") == 1, "cible fr: fr loaded");
        CHECK(lc.cibles().count("en") == 1, "cible fr: en loaded as fallback");
        CHECK(lc.cibles().count("de") == 0, "cible fr: de NOT loaded");
        CHECK(lc.cibles().count("es") == 0, "cible fr: es NOT loaded");
        CHECK(lc.cibles().count("it") == 0, "cible fr: it NOT loaded");
        CHECK(lc.cibles().count("nl") == 0, "cible fr: nl NOT loaded");
        CHECK(lc.cibles().count("pt") == 0, "cible fr: pt NOT loaded");
    }

    // --- Test 3: cible "de" loads de + fr + en ---
    {
        LemCore lc(RES, "de");
        CHECK(lc.cibles().count("de") == 1, "cible de: de loaded");
        CHECK(lc.cibles().count("fr") == 1, "cible de: fr loaded as fallback");
        CHECK(lc.cibles().count("en") == 1, "cible de: en loaded as fallback");
        CHECK(lc.cibles().count("es") == 0, "cible de: es NOT loaded");
    }

    // --- Test 4: fallback works at query time ---
    // "amor" exists in lemmes.la and has French + English translations.
    // When only "fr" + "en" are loaded, querying "it" falls back to "fr".
    {
        LemCore lc(RES, "fr");
        Lemme *l = lc.lemme("amor");
        CHECK(l != nullptr, "fallback test: amor lemma found");
        if (l) {
            std::string tr_fr = l->traduction("fr");
            std::string tr_it = l->traduction("it"); // it not loaded → fallback
            CHECK(!tr_fr.empty() && tr_fr != "non traduit / Translation not available.",
                  "fallback test: fr translation available");
            CHECK(tr_it == tr_fr,
                  "fallback test: querying unloaded 'it' returns fr translation");
        }
    }

    // --- Test 5: cible "en" only → fallback chain reaches en ---
    {
        LemCore lc(RES, "en");
        Lemme *l = lc.lemme("amor");
        CHECK(l != nullptr, "en-only: amor found");
        if (l) {
            std::string tr_en = l->traduction("en");
            CHECK(!tr_en.empty() && tr_en != "non traduit / Translation not available.",
                  "en-only: en translation available");
        }
    }

    // --- Test 6: dextrō (macron on o = \xC5\x8D) — must find dexter, not crash ---
    // Without the fix this caused infinite recursion (stack overflow): splitWordBoundary
    // split "dextrō" into ASCII "dextr" + bare ō byte-pair, and lemmatiseM(\xC5\x8D)
    // looped forever via toLower/toUpper byte corruption.
    {
        LemCore lc(RES, "fr");
        MapLem r = lc.lemmatiseM("dextr\xC5\x8D", true);   // dextrō
        CHECK(!r.empty(), "dextr\xC5\x8D (dextrō): at least one result (no crash)");
        bool found_dexter = false;
        for (auto &kv : r)
            if (kv.first->gr() == "dexter") found_dexter = true;
        CHECK(found_dexter, "dextr\xC5\x8D (dextrō): lemma is dexter");
    }

    // --- Test 7: dextrō via lemmatiseM with debPhr=false (same invariant) ---
    {
        LemCore lc(RES, "fr");
        MapLem r = lc.lemmatiseM("dextr\xC5\x8D", false);
        CHECK(!r.empty(), "dextr\xC5\x8D debPhr=false: at least one result");
    }

    RUN_TESTS();
}
