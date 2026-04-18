/*          modele.cpp  */

#include "modele.h"
#include "lemCore.h"
#include <regex>
#include <algorithm>

///////////////
// DESINENCE //
///////////////

Desinence::Desinence(const std::string &d, int morph, int nr, Modele *parent)
{
    int der = -1;
    if (!d.empty()) {
        char last = d.back();
        if (isdigit((unsigned char)last)) der = last - '0';
    }
    std::string ds = d;
    if (der > 0) {
        _rarete = der;
        ds.pop_back();
    } else {
        _rarete = 10;
    }
    if (ds == "-") ds = "";
    _grq = ds;
    _gr = Ch::atone(_grq);
    _morpho = morph;
    _numR = nr;
    _modele = parent;
}

std::string Desinence::gr()      { return _gr; }
std::string Desinence::grq()     { return _grq; }
Modele *Desinence::modele()      { return _modele; }
int Desinence::morphoNum()       { return _morpho; }
int Desinence::numRad()          { return _numR; }
int Desinence::rarete()          { return _rarete; }
void Desinence::setModele(Modele *m) { _modele = m; }

////////////
// MODELE //
////////////

const std::vector<std::string> Modele::cles = {
    "modele",  // 0
    "pere",    // 1
    "des",     // 2
    "des+",    // 3
    "R",       // 4
    "abs",     // 5
    "suf",     // 6
    "sufd",    // 7
    "abs+",    // 8
    "pos",     // 9
    "nbr"      // 10
};

Modele::Modele(std::vector<std::string> ll, LemCore *parent)
{
    _lemCore = parent;
    _pere = nullptr;
    _pos = '\0';
    _nbr = 0;
    std::multimap<std::string, int> msuff;

    // Regex for variable substitution: [:;](\w*)\+?(\$\w+)
    std::regex re("[:;]([\\w]*)\\+{0,1}(\\$\\w+)");

    for (auto &line : ll) {
        std::string l = line;
        // Substitute variables
        std::smatch m;
        while (std::regex_search(l, m, re)) {
            std::string v   = m[2].str();      // e.g. "$VAR"
            std::string var = _lemCore->variable(v);
            std::string pre = m[1].str();
            if (!pre.empty())
                var = replaceAll(var, ";", ";" + pre);
            l = replaceAll(l, v, var);
        }

        std::vector<std::string> eclats = split(simplified(l), ':');
        if (eclats.empty()) continue;

        auto it = std::find(cles.begin(), cles.end(), eclats.front());
        int p = (it != cles.end()) ? (int)(it - cles.begin()) : -1;

        switch (p) {
        case 0: // modele
            _gr = eclats.at(1);
            break;
        case 1: // pere
            _pere = parent->modele(eclats.at(1));
            break;
        case 2: // des
        case 3: // des+
        {
            std::vector<int> li = listeI(eclats.at(1));
            int r = toInt(eclats.at(2));
            std::vector<std::string> ld = split(eclats.at(3), ';');
            for (size_t i = 0; i < li.size(); ++i) {
                std::vector<std::string> ldd;
                if (i < ld.size())
                    ldd = split(ld.at(i), ',');
                else
                    ldd = split(ld.back(), ',');
                for (auto &g : ldd) {
                    Desinence *nd = new Desinence(g, li.at(i), r, this);
                    _desinences.insert({nd->morphoNum(), nd});
                    _lemCore->ajDesinence(nd);
                }
            }
            // des+ : also inherit from father for those morphos
            if (p == 3 && _pere != nullptr) {
                for (int i : li) {
                    for (Desinence *dp : _pere->desinences(i)) {
                        Desinence *dh = clone(dp);
                        _desinences.insert({i, dh});
                        _lemCore->ajDesinence(dh);
                    }
                }
            }
            break;
        }
        case 4: // R:n:gen
        {
            int nr = toInt(eclats.at(1));
            _genRadicaux[nr] = eclats.at(2);
            break;
        }
        case 8: // abs+
        {
            auto extra = listeI(eclats.at(1));
            _absents.insert(_absents.end(), extra.begin(), extra.end());
            break;
        }
        case 5: // abs
            _absents = listeI(eclats.at(1));
            break;
        case 6: // suf
        {
            std::vector<int> lsuf = listeI(eclats.at(1));
            std::string gr = eclats.at(2);
            for (int mi : lsuf)
                msuff.insert({gr, mi});
            break;
        }
        case 7: // sufd
        {
            if (_pere != nullptr) {
                _suf = eclats.at(1);
                for (Desinence *d : _pere->desinences()) {
                    if (std::find(_absents.begin(), _absents.end(), d->morphoNum()) != _absents.end())
                        continue;
                    std::string nd = d->grq();
                    Ch::allonge(&nd);
                    Desinence *dsuf = new Desinence(nd + _suf, d->morphoNum(), d->numRad(), this);
                    _desinences.insert({dsuf->morphoNum(), dsuf});
                    _lemCore->ajDesinence(dsuf);
                }
            }
            break;
        }
        case 9: // pos
            if (!eclats.at(1).empty()) _pos = eclats.at(1)[0];
            break;
        case 10: // nbr
            _nbr = toInt(eclats.at(1));
            break;
        default:
            break;
        }
    }

    // Inherit from father
    if (_pere != nullptr) {
        if (_pos == '\0') _pos = _pere->pos();
        for (int mi : _pere->morphos()) {
            if (deja(mi)) continue;
            for (Desinence *d : _pere->desinences(mi)) {
                if (std::find(_absents.begin(), _absents.end(), d->morphoNum()) != _absents.end())
                    continue;
                Desinence *dh = clone(d);
                _desinences.insert({dh->morphoNum(), dh});
                _lemCore->ajDesinence(dh);
            }
        }
        // inherit radical generators
        for (auto &kv : _desinences) {
            int nr = kv.second->numRad();
            if (_genRadicaux.find(nr) == _genRadicaux.end()) {
                std::string gen = _pere->genRadical(nr);
                _genRadicaux[nr] = gen;
            }
        }
        _absents = _pere->absents();
    }

    // Generate suffixed desinences
    std::vector<Desinence *> ldsuf;
    std::vector<std::string> clefsSuff = mm_unique_keys(msuff);
    for (auto &suff : clefsSuff) {
        std::vector<int> vals = mm_values(msuff, suff);
        for (auto &kv : _desinences) {
            Desinence *d = kv.second;
            if (std::find(vals.begin(), vals.end(), d->morphoNum()) != vals.end()) {
                std::string gq = d->grq();
                if (gq == "-") gq.clear();
                gq += suff;
                Desinence *dsuf = new Desinence(gq, d->morphoNum(), d->numRad(), this);
                ldsuf.push_back(dsuf);
            }
        }
    }
    for (Desinence *dsuf : ldsuf) {
        _desinences.insert({dsuf->morphoNum(), dsuf});
        _lemCore->ajDesinence(dsuf);
    }
}

bool Modele::absent(int a)
{
    return std::find(_absents.begin(), _absents.end(), a) != _absents.end();
}

std::vector<int> Modele::absents() { return _absents; }

std::vector<int> Modele::clesR()
{
    return map_keys(_genRadicaux);
}

Desinence *Modele::clone(Desinence *d)
{
    return new Desinence(d->grq() + _suf, d->morphoNum(), d->numRad(), this);
}

bool Modele::deja(int m)
{
    return _desinences.count(m) > 0;
}

std::vector<Desinence *> Modele::desinences(int d)
{
    return mm_values(_desinences, d);
}

std::vector<Desinence *> Modele::desinences()
{
    return mm_all_values(_desinences);
}

bool Modele::estUn(const std::string &m)
{
    if (_gr == m) return true;
    if (_pere == nullptr) return false;
    return _pere->estUn(m);
}

std::string Modele::gr() { return _gr; }

std::vector<int> Modele::listeI(const std::string &l)
{
    std::vector<int> result;
    std::vector<std::string> lvirg = split(l, ',');
    for (auto &virg : lvirg) {
        if (contains(virg, '-') && !virg.empty() && virg[0] != '-') {
            int deb = toInt(section(virg, '-', 0, 0));
            int fin = toInt(section(virg, '-', 1, 1));
            for (int i = deb; i <= fin; ++i) result.push_back(i);
        } else {
            result.push_back(toInt(virg));
        }
    }
    return result;
}

std::vector<int> Modele::morphos()
{
    return mm_unique_keys(_desinences);
}

char Modele::pos()
{
    if (_pos == '\0') return 'd';
    return _pos;
}

std::string Modele::genRadical(int r)
{
    auto it = _genRadicaux.find(r);
    return it != _genRadicaux.end() ? it->second : "";
}

int Modele::nbr() { return _nbr; }
