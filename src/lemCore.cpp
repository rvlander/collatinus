/*      lemCore.cpp  */

#include "lemCore.h"
#include "irregs.h"
#include "lemme.h"
#include "modele.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <cstring>

// -------------------------------------------------------------------------
// Constructor
// -------------------------------------------------------------------------
LemCore::LemCore(const std::string &resDir)
{
    Lemme::setLemCore(this);
    setResourceDir(resDir);

    _extension = false;
    _extLoaded = false;
    _medieval  = false;
    _cible     = "fr en es";

    suffixes["ne"]  = "n\xC4\x95";   // nĕ
    suffixes["que"] = "qu\xC4\x95";  // quĕ
    suffixes["ue"]  = "v\xC4\x95";   // vĕ
    suffixes["ve"]  = "v\xC4\x95";   // vĕ
    suffixes["st"]  = "st";

    ajAssims();
    ajAbrev();
    ajContractions();

    // Find all morphos.XX files
    std::vector<std::string> mfiles = listDirByPrefix(_resDir, "morphos.");
    // Remove morphos.la if present
    mfiles.erase(std::remove(mfiles.begin(), mfiles.end(), "morphos.la"), mfiles.end());
    for (auto &nfl : mfiles)
        lisMorphos(fileExtension(nfl));

    lisModeles();
    lisLexique();
    lisTags(false);
    lisTraductions(true, false);
    lisIrreguliers();
}

void LemCore::setResourceDir(const std::string &path)
{
    if (path.empty()) {
        _resDir = "./data/";
    } else if (endsWith(path, "/")) {
        _resDir = path;
    } else {
        _resDir = path + "/";
    }
}

// -------------------------------------------------------------------------
// lignesFichier — read file lines, skip empty and comments
// -------------------------------------------------------------------------
std::vector<std::string> LemCore::lignesFichier(const std::string &nf)
{
    return readFileLines(nf);
}

// -------------------------------------------------------------------------
// lisTags
// -------------------------------------------------------------------------
void LemCore::lisTags(bool tout)
{
    _tagOcc.clear();
    _tagTot.clear();
    _trigram.clear();
    auto lignes = lignesFichier(_resDir + "tags.la");
    size_t i = 0;
    for (; i < lignes.size(); ++i) {
        const std::string &l = lignes[i];
        if (startsWith(l, "! --- ")) break;
        auto ecl = split(l, ',');
        if (ecl.size() < 2) continue;
        _tagOcc[ecl[0]] += toInt(ecl[1]);
        _tagTot[ecl[0].substr(0,1)] += toInt(ecl[1]);
    }
    if (tout) {
        ++i;
        for (; i < lignes.size(); ++i) {
            const std::string &l = lignes[i];
            if (startsWith(l, "! --- ")) break;
            auto ecl = split(l, ',');
            if (ecl.size() < 2) continue;
            _trigram[ecl[0]] = toInt(ecl[1]);
        }
    }
}

// -------------------------------------------------------------------------
// tag
// -------------------------------------------------------------------------
std::string LemCore::tag(const std::string &lpIn, int m)
{
    std::string lp = lpIn;
    if (!lp.empty() && !isalpha((unsigned char)lp[0])) lp = "";
    std::string lTags;
    std::string morph = morpho(m);
    while (!lp.empty()) {
        std::string p = lp.substr(0,1);
        lp = lp.substr(1);
        if (p == "n" && m == 413) {
            lTags += "n71,";
        } else if (p == "v" && contains(morph, " -u")) {
            lTags += "v3 ,";
        } else if (!p.empty()) {
            std::string tag3 = p + "  ";
            if (p == "v") {
                for (int i = 0; i < 4; ++i) {
                    if (contains(morph, modes(i).substr(0, 3) /* partial match */)) {
                        // simplification: use mode index
                    }
                }
                // simplified tag computation
                for (int i = 0; i < 4; ++i) {
                    std::string md = modes(i);
                    if (!md.empty()) md = toLower(md);
                    if (contains(morph, md)) {
                        std::string t1 = std::to_string(i+1);
                        std::string t2 = contains(morph, temps(0)) ? "1" : " ";
                        lTags += p + t1 + t2 + ",";
                        goto next_pos;
                    }
                }
            }
            {
                bool found = false;
                for (int i = 0; i < 6; ++i) {
                    if (contains(morph, cas(i))) {
                        std::string t1 = std::to_string(i+1);
                        std::string t2 = contains(morph, nombre(1)) ? "2" : "1";
                        std::string pp = p;
                        if (pp == "v") pp = "w";
                        lTags += pp + t1 + t2 + ",";
                        found = true;
                        break;
                    }
                }
                if (!found) lTags += p + "  ,";
            }
            next_pos:;
        }
    }
    return lTags;
}

std::string LemCore::tag(Lemme *l, int m)
{
    return tag(l->pos(), m);
}

int LemCore::fraction(const std::string &listTagsIn)
{
    std::string listTags = listTagsIn;
    int frFin = 0;
    while (listTags.size() > 2) {
        std::string t = listTags.substr(0,3);
        if (listTags.size() > 3) listTags = listTags.substr(4);
        else break;
        int fr = 0;
        if (_tagOcc.count(t)) {
            char p = t[0];
            int tot = _tagTot.count(t.substr(0,1)) ? _tagTot.at(t.substr(0,1)) : 1;
            if (tot == 0) tot = 1;
            if (p=='a'||p=='p'||p=='w') fr = _tagOcc.at(t)*341/tot;
            else if (p=='v' && t[2]=='1') fr = _tagOcc.at(t)*256/tot;
            else if (p=='v' && t[2]==' ') fr = _tagOcc.at(t)*128/tot;
            else if (p=='n') fr = _tagOcc.at(t)*1024/tot;
            else fr = 1024;
            if (fr == 0) fr = 1;
        }
        if (frFin < fr) frFin = fr;
    }
    return (frFin == 0) ? 1024 : frFin;
}

int LemCore::tagOcc(const std::string &t) { return map_value(_tagOcc, t, 0); }

int LemCore::trigram(const std::string &seq)
{
    if (_trigram.empty()) lisTags(true);
    return map_value(_trigram, seq, 0);
}

// -------------------------------------------------------------------------
// lisMorphos
// -------------------------------------------------------------------------
void LemCore::lisMorphos(const std::string &lang)
{
    auto lignes = lignesFichier(_resDir + "morphos." + lang);
    size_t i = 0;
    std::vector<std::string> morphos;
    for (; i < lignes.size(); ++i) {
        const std::string &l = lignes[i];
        if (startsWith(l, "! --- ")) break;
        morphos.push_back(section(l, ':', 1, 1));
    }
    _morphos[lang] = morphos;
    ++i;

    auto readBlock = [&](std::vector<std::string> &out) {
        out.clear();
        for (; i < lignes.size(); ++i) {
            if (startsWith(lignes[i], "! --- ")) { ++i; break; }
            out.push_back(lignes[i]);
        }
    };
    readBlock(_cas[lang]);
    readBlock(_genres[lang]);
    readBlock(_nombres[lang]);
    readBlock(_temps[lang]);
    readBlock(_modes[lang]);
    readBlock(_voix[lang]);
    readBlock(_motsClefs[lang]);
}

// -------------------------------------------------------------------------
// ajAssims
// -------------------------------------------------------------------------
void LemCore::ajAssims()
{
    auto lignes = lignesFichier(_resDir + "assimilations.la");
    for (auto &lin : lignes) {
        auto liste = split(lin, ':');
        if (liste.size() < 2) continue;
        assimsq[liste[0]] = liste[1];
        assims[Ch::atone(liste[0])] = Ch::atone(liste[1]);
    }
}

void LemCore::ajAbrev()
{
    abr = lignesFichier(_resDir + "abreviations.la");
}

bool LemCore::estAbr(const std::string &m)
{
    return std::find(abr.begin(), abr.end(), m) != abr.end();
}

// -------------------------------------------------------------------------
// ajContractions
// -------------------------------------------------------------------------
void LemCore::ajContractions()
{
    auto lignes = lignesFichier(_resDir + "contractions.la");
    for (auto &lin : lignes) {
        auto liste = split(lin, ':');
        if (liste.size() < 2) continue;
        _contractions[liste[0]] = liste[1];
    }
}

// -------------------------------------------------------------------------
// aRomano
// -------------------------------------------------------------------------
int LemCore::aRomano(const std::string &f)
{
    if (f.empty()) return 0;
    std::map<char,int> conv;
    conv['I']=1; conv['V']=5; conv['U']=5;
    conv['X']=10; conv['L']=50; conv['C']=100;
    conv['D']=500; conv['M']=1000;
    int res = 0;
    int conv_s = conv.count(f[0]) ? conv.at(f[0]) : 0;
    for (size_t i = 0; i+1 < f.size(); ++i) {
        int conv_c = conv_s;
        conv_s = conv.count(f[i+1]) ? conv.at(f[i+1]) : 0;
        if (conv_c < conv_s) res -= conv_c;
        else res += conv_c;
    }
    res += conv_s;
    return res;
}

// -------------------------------------------------------------------------
// ajDesinence
// -------------------------------------------------------------------------
void LemCore::ajDesinence(Desinence *d)
{
    _desinences.insert({Ch::deramise(d->gr()), d});
}

// -------------------------------------------------------------------------
// estRomain
// -------------------------------------------------------------------------
bool LemCore::estRomain(const std::string &f)
{
    if (f.empty()) return false;
    static const std::regex reNotRoman("[^IUXLCDM]");
    return !std::regex_search(f, reNotRoman)
        && !contains(f, "IL")
        && !contains(f, "IUI");
}

// -------------------------------------------------------------------------
// ajRadicaux
// -------------------------------------------------------------------------
void LemCore::ajRadicaux(Lemme *l)
{
    Modele *m = modele(l->grModele());
    if (!m) return;
    for (int i : l->clesR()) {
        for (Radical *r : l->radical(i))
            _radicaux.insert({Ch::deramise(r->gr()), r});
    }
    for (int i : m->clesR()) {
        auto clesL = l->clesR();
        if (std::find(clesL.begin(), clesL.end(), i) != clesL.end()) continue;
        std::vector<std::string> gs = split(l->grq(), ',');
        for (auto &g : gs) {
            std::string gen = m->genRadical(i);
            if (gen == "-") continue;
            Radical *r = nullptr;
            std::string gc = g;
            if (gen == "K") {
                r = new Radical(gc, i, l);
            } else {
                int oter = toInt(section(gen, ',', 0, 0));
                std::string ajouter = section(gen, ',', 1, 1);
                if (endsWith(gc, "\xCC\x86")) gc.erase(gc.size()-2); // drop combining breve
                if (oter > 0 && (int)gc.size() >= oter)
                    gc.erase(gc.size() - oter);
                if (ajouter != "0") gc += ajouter;
                r = new Radical(gc, i, l);
            }
            if (r) {
                l->ajRadical(i, r);
                _radicaux.insert({Ch::deramise(r->gr()), r});
            }
        }
    }
}

// -------------------------------------------------------------------------
// assim / desassim
// -------------------------------------------------------------------------
std::string LemCore::assim(const std::string &a)
{
    for (auto &kv : assims)
        if (startsWith(a, kv.first))
            return replaceAll(a, kv.first, kv.second);
    return a;
}

std::string LemCore::assimq(const std::string &a)
{
    for (auto &kv : assimsq)
        if (startsWith(a, kv.first))
            return replaceAll(a, kv.first, kv.second);
    return a;
}

std::string LemCore::desassim(const std::string &a)
{
    for (auto &kv : assims)
        if (startsWith(a, kv.second))
            return replaceAll(a, kv.second, kv.first);
    return a;
}

std::string LemCore::desassimq(const std::string &a)
{
    for (auto &kv : assimsq)
        if (startsWith(a, kv.second))
            return replaceAll(a, kv.second, kv.first);
    return a;
}

std::string LemCore::cible() { return _cible; }
void LemCore::setCible(const std::string &c) { _cible = c; }
std::map<std::string, std::string> LemCore::cibles() { return _cibles; }

std::string LemCore::decontracte(const std::string &din)
{
    std::string d = din;
    for (auto &kv : _contractions) {
        if (endsWith(d, kv.first)) {
            d.erase(d.size() - kv.first.size());
            if (contains(d, "v") || contains(d, "V"))
                d += kv.second;
            else
                d += Ch::deramise(kv.second);
            return d;
        }
    }
    return d;
}

// -------------------------------------------------------------------------
// inconnue
// -------------------------------------------------------------------------
ModLem LemCore::inconnue(const std::string &f)
{
    ModLem result;
    if (f.empty()) return result;
    bool que = (f.size() > 3) && endsWith(f, "que");
    std::string ff = que ? f.substr(0, f.size()-3) : f;
    ff = Ch::deramise(ff);
    if (_medieval) { ff = transfMed(ff); if (ff.empty()) return result; }
    std::string nn = " (%1)";
    std::string ppp = "tsxu";
    for (size_t i = 1; i <= ff.size(); ++i) {
        std::string r = ff.substr(0, i);
        std::string d = ff.substr(i);
        auto ldes = mm_values(_desinences, d);
        if (_medieval && _desMed.count(d))
            for (auto *x : mm_values(_desinences, _desMed.at(d))) ldes.push_back(x);
        if (ldes.empty()) continue;
        auto lrad = mm_values(_radicaux, r);
        if (_medieval && _radMed.count(r))
            for (auto &rm : mm_values(_radMed, r))
                for (auto *x : mm_values(_radicaux, rm)) lrad.push_back(x);
        for (Desinence *des : ldes) {
            if (des->modele()->nbr() <= 0) continue;
            int nm = des->morphoNum();
            bool OK = true;
            if (nm>84&&nm<121 && !endsWith(r,"im") && !contains(d,"im")) OK=false;
            if (nm>48&&nm<85  && !endsWith(r,"i")  && !startsWith(d,"i")) OK=false;
            if (nm>302&&nm<339) {
                std::string last1 = r.empty() ? "" : r.substr(r.size()-1);
                if (!contains(ppp, last1)) OK=false;
            }
            if (OK) {
                std::string r1 = r + " (" + std::to_string(des->numRad()) + ")";
                SLem sl = {r1, nm, que ? des->grq() + "qu\xC4\x95" : des->grq()};
                result[des->modele()].insert(result[des->modele()].begin(), sl);
            }
        }
    }
    return result;
}

// Forward declaration — defined after lemmatise/lemmatiseM below
static int countOcc(const std::string &s, const std::string &sub);

// -------------------------------------------------------------------------
// lemmatise
// -------------------------------------------------------------------------
MapLem LemCore::lemmatise(const std::string &f)
{
    MapLem result;
    if (f.empty()) return result;
    std::string f_lower = toLower(f);
    int cnt_v   = (int)countOcc(f_lower, "v");
    bool V_maj  = (!f.empty() && f[0] == 'V');
    int cnt_ae  = (int)countOcc(f_lower, "\xC3\xA6"); // æ
    int cnt_oe  = (int)countOcc(f_lower, "\xC5\x93"); // œ
    if (endsWith(f_lower, "\xC3\xA6")) cnt_ae--;

    std::string fd = Ch::deramise(f);
    if (_medieval) { fd = transfMed(fd); if (fd.empty()) return result; }

    // formes irrégulières
    for (Irreg *irr : mm_values(_irregs, fd)) {
        for (int m : irr->morphos()) {
            SLem sl = {irr->grq(), m, ""};
            result[irr->lemme()].insert(result[irr->lemme()].begin(), sl);
        }
    }
    if (_medieval && _irrMed.count(fd))
        for (Irreg *irr : mm_values(_irregs, _irrMed.at(fd)))
            for (int m : irr->morphos()) {
                SLem sl = {irr->grq(), m, ""};
                result[irr->lemme()].insert(result[irr->lemme()].begin(), sl);
            }

    // radical + désinence
    for (size_t i = 0; i <= fd.size(); ++i) {
        std::string r = fd.substr(0, i);
        std::string d = fd.substr(i);
        auto ldes = mm_values(_desinences, d);
        if (_medieval && _desMed.count(d))
            for (auto *x : mm_values(_desinences, _desMed.at(d))) ldes.push_back(x);
        if (ldes.empty()) continue;

        auto lrad = mm_values(_radicaux, r);
        if (_medieval && _radMed.count(r))
            for (auto &rm : mm_values(_radMed, r))
                for (auto *x : mm_values(_radicaux, rm)) lrad.push_back(x);
        // ii noté ī
        if (!d.empty() && d[0]=='i' && (d.size()<2||d[1]!='i') && !endsWith(r,"i"))
            for (auto *x : mm_values(_radicaux, r+"i")) lrad.push_back(x);
        if (_medieval && (d.empty() || d[0]=='i')) {
            std::string rbis = r;
            if (d.empty() && endsWith(r,"ti")) rbis[rbis.size()-2]='c';
            if (!d.empty() && d[0]=='i' && d.size()>1 && endsWith(r,"c")) {
                const std::string voy = "aeiouy";
                if (contains(voy, std::string(1,d[1]))) rbis[rbis.size()-1]='t';
            }
            if (r != rbis) {
                for (auto *x : mm_values(_radicaux, rbis)) lrad.push_back(x);
                if (_radMed.count(rbis))
                    for (auto &rm : mm_values(_radMed, rbis))
                        for (auto *x : mm_values(_radicaux, rm)) lrad.push_back(x);
            }
        }
        if (lrad.empty()) continue;

        for (Radical *rad : lrad) {
            Lemme *l = rad->lemme();
            for (Desinence *des : ldes) {
                if (des->modele() == l->modele() &&
                    des->numRad() == rad->numRad() &&
                    !l->estIrregExcl(des->morphoNum()))
                {
                    int cnt_v_rad = (int)countOcc(toLower(rad->grq()), "v")
                                  + (int)countOcc(des->grq(), "v");
                    bool c = (cnt_v==0) || (cnt_v == cnt_v_rad);
                    if (!c) c = (V_maj && rad->gr().size()>0 && rad->gr()[0]=='U'
                                 && cnt_v-1 == (int)countOcc(toLower(rad->grq()),"v"));
                    c = c && ((cnt_oe==0)||(cnt_oe==(int)countOcc(toLower(rad->grq()),"\xC5\x8D""e")));
                    c = c && ((cnt_ae==0)||
                              (cnt_ae==(int)(countOcc(toLower(rad->grq()),"\xC4\x81""e")
                                           +countOcc(toLower(rad->grq()),"pr\xC4\x83""e"))));
                    if (c || _medieval) {
                        std::string fq = rad->grq() + des->grq();
                        if (!endsWith(r,"i") && endsWith(rad->gr(),"i")) {
                            std::string rq = rad->grq();
                            rq.erase(rq.size()-1);
                            std::string dq = des->grq();
                            if (!dq.empty()) dq = dq.substr(1);
                            fq = rq + "\xC4\xAB" + dq; // ī
                        }
                        SLem sl = {fq, des->morphoNum(), ""};
                        result[l].insert(result[l].begin(), sl);
                    }
                }
            }
        }
    }

    if (_extLoaded && !_extension && !result.empty()) {
        MapLem res;
        for (auto &kv : result)
            if (kv.first->origin() == 0) res[kv.first] = kv.second;
        if (!res.empty()) result = res;
    }

    // Chiffres romains
    if (estRomain(fd) && !_lemmes.count(fd)) {
        std::string f1 = fd;
        std::string fdv = replaceChar(fd, 'U', 'V');
        std::string lin = fdv + "|inv|||adj. num.|1";
        Lemme *romain = new Lemme(lin, 0);
        int nr = aRomano(fd);
        romain->ajTrad(std::to_string(nr), "fr");
        _lemmes[f1] = romain;
        SLem sl = {f1, 416, ""};
        result[romain].push_back(sl);
    }
    return result;
}

// -------------------------------------------------------------------------
// countOcc: count occurrences of sub in s
// -------------------------------------------------------------------------
static int countOcc(const std::string &s, const std::string &sub)
{
    int n = 0; size_t pos = 0;
    while ((pos = s.find(sub, pos)) != std::string::npos) { ++n; pos += sub.size(); }
    return n;
}

// -------------------------------------------------------------------------
// inv
// -------------------------------------------------------------------------
bool LemCore::inv(Lemme *l, const MapLem &ml)
{
    auto it = ml.find(l);
    if (it == ml.end() || it->second.empty()) return false;
    return it->second.at(0).morpho == 416;
}

// -------------------------------------------------------------------------
// lemmatiseM
// -------------------------------------------------------------------------
MapLem LemCore::lemmatiseM(const std::string &f, bool debPhr, int etape)
{
    MapLem mm;
    if (f.empty()) return mm;
    if (etape > 3 || etape < 0) {
        mm = lemmatise(f);
        if ((debPhr && !f.empty() && isupper((unsigned char)f[0])) ||
            (mm.empty() && f == toUpper(f) && !f.empty() && !isdigit((unsigned char)f[0]) && f.size()>1))
        {
            std::string nf = toLower(f);
            MapLem nmm = lemmatiseM(nf);
            for (auto &kv : nmm) mm[kv.first] = kv.second;
        }
        return mm;
    }
    mm = lemmatiseM(f, debPhr, etape+1);
    std::string fd;
    switch (etape) {
    case 3:
        fd = f;
        for (auto &kv : _contractions) {
            if (endsWith(fd, kv.first)) {
                fd.erase(fd.size() - kv.first.size());
                if (contains(fd,"v")||contains(fd,"V"))
                    fd += kv.second;
                else
                    fd += Ch::deramise(kv.second);
                MapLem nmm = lemmatiseM(fd, debPhr, 4);
                for (auto &nl : nmm) {
                    int diff = (int)kv.second.size() - (int)kv.first.size();
                    for (size_t i = 0; i < nl.second.size(); ++i) {
                        int position = (int)f.size() - (int)kv.first.size() + 1;
                        if ((int)fd.size() != (int)nl.second[i].grq.size()) {
                            std::string debut = nl.second[i].grq.substr(0, position+2);
                            position += (int)countOcc(debut, "\xCC\x86");
                        }
                        nl.second[i].grq.erase(position, diff);
                    }
                    mm[nl.first] = nl.second;
                }
                break;
            }
        }
        break;
    case 2:
        fd = assim(f);
        if (fd != f) {
            MapLem nmm = lemmatiseM(fd, debPhr, 3);
            for (auto &nl : nmm) {
                for (auto &sl : nl.second) sl.grq = desassimq(sl.grq);
                mm[nl.first] = nl.second;
            }
            return mm;
        }
        fd = desassim(f);
        if (fd != f) {
            MapLem nmm = lemmatiseM(fd, debPhr, 3);
            for (auto &nl : nmm) {
                for (auto &sl : nl.second) sl.grq = assimq(sl.grq);
                mm[nl.first] = nl.second;
            }
            return mm;
        }
        break;
    case 1:
        if (mm.empty())
            for (auto &kv : suffixes) {
                if (mm.empty() && endsWith(f, kv.first)) {
                    std::string sf = f.substr(0, f.size() - kv.first.size());
                    mm = lemmatiseM(sf, debPhr, 1);
                    bool sst = false;
                    if (mm.empty() && kv.first == "st") {
                        sf += "s";
                        mm = lemmatiseM(sf, debPhr, 1);
                        sst = true;
                    }
                    for (auto &l2 : mm) {
                        for (size_t i = 0; i < l2.second.size(); ++i)
                            if (sst) mm[l2.first][i].sufq = "t";
                            else mm[l2.first][i].sufq += kv.second;
                    }
                }
            }
        break;
    case 0:
        if (mm.empty() && !f.empty() && islower((unsigned char)f[0])) {
            std::string ff = f;
            ff[0] = (char)toupper((unsigned char)ff[0]);
            return lemmatiseM(ff, false, 1);
        }
        break;
    default:
        break;
    }
    return mm;
}

// -------------------------------------------------------------------------
// Accessors
// -------------------------------------------------------------------------
Lemme *LemCore::lemme(const std::string &l) { return map_value(_lemmes, l, (Lemme*)nullptr); }

int LemCore::nbOcc(const std::string &l)
{
    auto it = _lemmes.find(l);
    return it != _lemmes.end() ? it->second->nbOcc() : 0;
}

std::vector<std::string> LemCore::lemmes(MapLem lm)
{
    std::vector<std::string> res;
    for (auto &kv : lm) res.push_back(kv.first->gr());
    return res;
}

// -------------------------------------------------------------------------
// lisIrreguliers
// -------------------------------------------------------------------------
void LemCore::lisIrreguliers()
{
    auto lignes = lignesFichier(_resDir + "irregs.la");
    for (auto &lin : lignes) {
        Irreg *irr = new Irreg(lin, this);
        if (irr && irr->lemme())
            _irregs.insert({Ch::deramise(irr->gr()), irr});
    }
    for (auto &kv : _irregs)
        kv.second->lemme()->ajIrreg(kv.second);
}

// -------------------------------------------------------------------------
// lisFichierLexique
// -------------------------------------------------------------------------
void LemCore::lisFichierLexique(const std::string &filepath)
{
    int orig = endsWith(filepath, "ext.la") ? 1 : 0;
    auto lignes = lignesFichier(filepath);
    for (auto &lin : lignes) {
        Lemme *l = new Lemme(lin, orig);
        _lemmes[l->cle()] = l;
    }
}

void LemCore::lisLexique()    { lisFichierLexique(_resDir + "lemmes.la"); }
void LemCore::lisExtension()  { lisFichierLexique(_resDir + "lem_ext.la"); }

// -------------------------------------------------------------------------
// lisModeles
// -------------------------------------------------------------------------
void LemCore::lisModeles()
{
    auto lignes = lignesFichier(_resDir + "modeles.la");
    std::vector<std::string> sl;
    for (size_t i = 0; i <= lignes.size(); ++i) {
        std::string l = (i < lignes.size()) ? lignes[i] : "modele:";
        if (startsWith(l, "$")) {
            _variables[section(l,'=',0,0)] = section(l,'=',1,1);
            continue;
        }
        auto eclats = split(l, ':');
        if ((eclats.front() == "modele" || i == lignes.size()) && !sl.empty()) {
            Modele *m = new Modele(sl, this);
            _modeles[m->gr()] = m;
            sl.clear();
        }
        if (i < lignes.size()) sl.push_back(l);
    }
}

// -------------------------------------------------------------------------
// lisTransfMed
// -------------------------------------------------------------------------
void LemCore::lisTransfMed()
{
    auto lignes = lignesFichier(_resDir + "medieval.txt");
    for (auto &ligne : lignes) {
        auto rr = split(ligne, ';');
        if (rr.size() < 2) continue;
        try {
            _reglesMed.push_back({std::regex(rr[0]), rr[1]});
        } catch (...) {}
    }
}

// -------------------------------------------------------------------------
// transfMed
// -------------------------------------------------------------------------
std::string LemCore::transfMed(const std::string &fin, bool rad)
{
    if (fin.empty()) return "";
    if (estRomain(fin)) return fin;
    bool maj = !fin.empty() && isupper((unsigned char)fin[0]);
    std::string f = toLower(fin);
    for (auto &r : _reglesMed) {
        if (!endsWith(r.second, "*"))
            f = std::regex_replace(f, r.first, r.second);
        else if (rad) {
            std::string rs = r.second.substr(0, r.second.size()-1);
            f = std::regex_replace(f, r.first, rs);
        }
    }
    if (maj && !f.empty()) f[0] = (char)toupper((unsigned char)f[0]);
    return f;
}

// -------------------------------------------------------------------------
// lisTraductions
// -------------------------------------------------------------------------
void LemCore::lisTraductions(bool base, bool extension)
{
    if (!base && !extension) return;
    std::vector<std::string> files;
    if (base && extension) {
        // lem*.* (both lemmes.XX and lem_ext.XX)
        auto all = listDirByPrefix(_resDir, "lem");
        for (auto &f : all) {
            if (f == "lemmes.la" || f == "lem_ext.la") continue;
            if (startsWith(f,"lemmes.") || startsWith(f,"lem_ext.")) files.push_back(f);
        }
    } else if (base) {
        auto all = listDirByPrefix(_resDir, "lemmes.");
        for (auto &f : all) { if (f != "lemmes.la") files.push_back(f); }
    } else {
        auto all = listDirByPrefix(_resDir, "lem_ext.");
        for (auto &f : all) { if (f != "lem_ext.la") files.push_back(f); }
    }

    for (auto &nfl : files) {
        std::string suff = fileExtension(nfl);
        auto lignes = lignesFichier(_resDir + nfl);
        if (lignes.empty()) continue;
        if (base && startsWith(nfl, "lemmes.")) {
            _cibles[suff] = lignes.front();
            lignes.erase(lignes.begin());
        }
        for (auto &lin : lignes) {
            Lemme *l = lemme(Ch::deramise(section(lin, ':', 0, 0)));
            if (l) l->ajTrad(section(lin, ':', 1), suff);
        }
    }
}

// -------------------------------------------------------------------------
// modele / morpho / cas / genre etc.
// -------------------------------------------------------------------------
Modele *LemCore::modele(const std::string &m)
{
    auto it = _modeles.find(m);
    return it != _modeles.end() ? it->second : nullptr;
}

std::string LemCore::morpho(int m)
{
    std::string l = "fr";
    if (_morphos.count(_cible.substr(0,2))) l = _cible.substr(0,2);
    else if (_cible.size()>4 && _morphos.count(_cible.substr(3,2))) l = _cible.substr(3,2);
    if (!_morphos.count(l)) return "-";
    auto &v = _morphos.at(l);
    if (m <= 0 || m > (int)v.size()) return "morpho," + std::to_string(m) + ":erreur";
    if (m == (int)v.size()) return "-";
    return v.at(m-1);
}

static std::string langKey(const std::string &cible,
                            const std::map<std::string,std::vector<std::string>> &m)
{
    if (m.count(cible.substr(0,2))) return cible.substr(0,2);
    if (cible.size()>4 && m.count(cible.substr(3,2))) return cible.substr(3,2);
    return "fr";
}

std::string LemCore::cas(int i)
{
    std::string l = langKey(_cible, _cas);
    if (!_cas.count(l)) return "";
    auto &v = _cas.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::genre(int i)
{
    std::string l = langKey(_cible, _genres);
    if (!_genres.count(l)) return "";
    auto &v = _genres.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::nombre(int i)
{
    std::string l = langKey(_cible, _nombres);
    if (!_nombres.count(l)) return "";
    auto &v = _nombres.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::temps(int i)
{
    std::string l = langKey(_cible, _temps);
    if (!_temps.count(l)) return "";
    auto &v = _temps.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::modes(int i)
{
    std::string l = langKey(_cible, _modes);
    if (!_modes.count(l)) return "";
    auto &v = _modes.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::voix(int i)
{
    std::string l = langKey(_cible, _voix);
    if (!_voix.count(l)) return "";
    auto &v = _voix.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}
std::string LemCore::motsClefs(int i)
{
    std::string l = langKey(_cible, _motsClefs);
    if (!_motsClefs.count(l)) return "";
    auto &v = _motsClefs.at(l);
    return (i>=0&&i<(int)v.size()) ? v[i] : "";
}

bool LemCore::optExtension() { return _extension; }

std::string LemCore::variable(const std::string &v)
{
    return map_value(_variables, v);
}

void LemCore::setExtension(bool e)
{
    _extension = e;
    if (!_extLoaded && e) {
        lisExtension();
        lisTraductions(false, true);
        _extLoaded = true;
    }
}

void LemCore::setMedieval(bool e)
{
    _medieval = e;
    if (_reglesMed.empty() && e) {
        lisTransfMed();
        auto liste = mm_unique_keys(_desinences);
        for (auto &clef : liste) {
            std::string cleMed = transfMed(clef);
            if (clef != cleMed) _desMed[cleMed] = clef;
        }
        auto listeI = mm_unique_keys(_irregs);
        for (auto &clef : listeI) {
            std::string cleMed = transfMed(clef);
            if (clef != cleMed) _irrMed[cleMed] = clef;
        }
        auto listeR = mm_unique_keys(_radicaux);
        for (auto &clef : listeR) {
            std::string cleMed = transfMed(clef, true);
            if (clef != cleMed) _radMed.insert({cleMed, clef});
        }
    }
}

void LemCore::lireHyphen(const std::string &fichierHyphen)
{
    for (auto &kv : _lemmes) kv.second->setHyphen("");
    if (fichierHyphen.empty()) return;
    auto lignes = lignesFichier(fichierHyphen);
    for (auto &linea : lignes) {
        auto ecl = split(linea, '|');
        if (ecl.size() < 2) continue;
        ecl[1] = replaceAll(ecl[1], "-", Ch::separSyll);
        Lemme *l = lemme(Ch::deramise(ecl[0]));
        if (l) l->setHyphen(ecl[1]);
    }
}
