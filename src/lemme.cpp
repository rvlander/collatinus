/*      lemme.cpp  */

#include "lemme.h"
#include "irregs.h"
#include "lemCore.h"
#include "modele.h"
#include <sstream>
#include <regex>
#include <algorithm>

LemCore *Lemme::_lemCore = nullptr;

/////////////
// RADICAL //
/////////////

Radical::Radical(const std::string &g, int n, Lemme *parent)
{
    _lemme  = parent;
    _grq    = Ch::communes(g);
    _gr     = Ch::atone(g);
    _numero = n;
}

std::string Radical::gr()       { return _gr; }
std::string Radical::grq() const { return _grq; }
Lemme *Radical::lemme()         { return _lemme; }
Modele *Radical::modele()       { return _lemme->modele(); }
int Radical::numRad()           { return _numero; }

///////////
// LEMME //
///////////

Lemme::Lemme(const std::string &linea, int origin)
{
    // cădo|lego|cĕcĭd|cās|is, ere, cecidi, casum|687
    std::vector<std::string> eclats = split(linea, '|');
    std::vector<std::string> lg     = split(eclats.at(0), '=');
    _nh     = 1;
    _cle    = Ch::atone(Ch::deramise(lg.at(0)));
    _grd    = oteNh(lg.at(0), _nh);
    _grq    = (lg.size() == 1) ? _grd : lg.at(1);
    _gr     = Ch::atone(section(_grq, ',', 0, 0));
    _grModele = eclats.at(1);
    _modele   = _lemCore->modele(_grModele);
    _hyphen   = "";
    _origin   = origin;
    _nbOcc    = 1;

    if ((int)eclats.size() < 6) {
        // mal formée — continue anyway
    }

    // Radicaux champs 2 et 3
    for (int i = 2; i < 4; ++i) {
        if (i >= (int)eclats.size()) break;
        if (!eclats.at(i).empty()) {
            std::vector<std::string> lrad = split(eclats.at(i), ',');
            for (auto &rad : lrad)
                _radicaux[i-1].push_back(new Radical(rad, i-1, this));
        }
    }
    _lemCore->ajRadicaux(this);

    _indMorph = (eclats.size() > 4) ? eclats.at(4) : "";

    // renvoi
    std::regex c("cf\\.\\s(\\w+)$");
    std::smatch sm;
    if (std::regex_search(_indMorph, sm, c))
        _renvoi = sm[1].str();
    else
        _renvoi = "";

    // POS
    _pos.clear();
    if (contains(_indMorph, "adj."))  _pos += 'a';
    if (contains(_indMorph, "conj"))  _pos += 'c';
    if (contains(_indMorph, "excl.")) _pos += 'e';
    if (contains(_indMorph, "interj")) _pos += 'i';
    if (contains(_indMorph, "num."))  _pos += 'm';
    if (contains(_indMorph, "pron.")) _pos += 'p';
    if (contains(_indMorph, "prép"))  _pos += 'r';
    if (contains(_indMorph, "adv"))   _pos += 'd';
    if (contains(_indMorph, " nom ") || contains(_indMorph, "npr.")) _pos += 'n';
    if (_pos.empty()) {
        _pos += _modele->pos();
        if (_pos == "d" && !_renvoi.empty()) _pos = "";
    }

    if (eclats.size() > 5)
        _nbOcc = toInt(eclats.at(5));
}

void Lemme::setLemCore(LemCore *l)
{
    if (_lemCore == nullptr) _lemCore = l;
}

void Lemme::ajIrreg(Irreg *irr)
{
    _irregs.push_back(irr);
    if (irr->exclusif()) {
        auto mv = irr->morphos();
        _morphosIrrExcl.insert(_morphosIrrExcl.end(), mv.begin(), mv.end());
    }
}

void Lemme::ajNombre(int n) { _nbOcc += n; }

void Lemme::ajRadical(int i, Radical *r) { _radicaux[i].push_back(r); }

void Lemme::ajTrad(const std::string &t, const std::string &l) { _traduction[l] = t; }

std::string Lemme::ambrogio()
{
    std::ostringstream ss;
    ss << "<hr/>" << humain() << "<br/>";
    ss << "<table>";
    for (auto &kv : _traduction) {
        const std::string &lang  = kv.first;
        const std::string &trad  = kv.second;
        std::string langue = map_value(_lemCore->cibles(), lang);
        if (!trad.empty())
            ss << "<tr><td>- " << langue << "</td><td>&nbsp;" << trad << "</td></tr>\n";
    }
    ss << "</table>";
    return ss.str();
}

std::string Lemme::cle() { return _cle; }

std::vector<int> Lemme::clesR()
{
    return map_keys(_radicaux);
}

bool Lemme::estIrregExcl(int nm)
{
    return std::find(_morphosIrrExcl.begin(), _morphosIrrExcl.end(), nm) != _morphosIrrExcl.end();
}

std::string Lemme::genre()
{
    std::string g;
    if (contains(_indMorph, " m.")) g += " " + _lemCore->genre(0);
    if (contains(_indMorph, " f.")) g += " " + _lemCore->genre(1);
    if (contains(_indMorph, " n.")) g += " " + _lemCore->genre(2);
    g = trim(g);
    if (!_renvoi.empty() && g.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        if (lr != nullptr) return lr->genre();
    }
    return g;
}

std::string Lemme::gr()  { return _gr; }
std::string Lemme::grq() { return _grq; }
std::string Lemme::grModele() { return _grModele; }

std::string Lemme::humain(bool html, const std::string &l, bool nbr)
{
    std::string res;
    std::string tr;
    if (!_renvoi.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        tr = (lr != nullptr) ? lr->traduction(l) : "renvoi non trouvé";
    } else {
        tr = traduction(l);
    }

    std::ostringstream flux;
    std::string grq = _grq;
    if (contains(grq, ",")) {
        grq = replaceAll(grq, ",", ", ");
        grq = replaceAll(grq, "  ", " ");
    }

    if (html) {
        if (_nh > 1)
            flux << "<strong>" << grq << "<sup>" << _nh << "</sup></strong>, "
                 << "<em>" << _indMorph << "</em>";
        else
            flux << "<strong>" << grq << "</strong>, "
                 << "<em>" << _indMorph << "</em>";
    } else {
        if (_nh > 1)
            flux << grq << "_" << _nh << ", " << _indMorph;
        else
            flux << grq << ", " << _indMorph;
    }

    if (_nbOcc != 1 && nbr) {
        if (html) flux << " <small>(" << _nbOcc << ")</small>";
        else      flux << " (" << _nbOcc << ")";
    }
    flux << " : " << tr;
    return flux.str();
}

std::string Lemme::indMorph() { return _indMorph; }

std::string Lemme::irreg(int i, bool *excl)
{
    for (Irreg *ir : _irregs) {
        auto mv = ir->morphos();
        if (std::find(mv.begin(), mv.end(), i) != mv.end()) {
            *excl = ir->exclusif();
            return ir->grq();
        }
    }
    return "";
}

Modele *Lemme::modele() { return _modele; }
int Lemme::nbOcc() const { return _nbOcc; }
void Lemme::clearOcc()   { _nbOcc = 1; }
int Lemme::nh()          { return _nh; }
int Lemme::origin()      { return _origin; }

std::string Lemme::oteNh(const std::string &g, int &nh)
{
    if (g.empty()) return g;
    char last = g.back();
    if (isdigit((unsigned char)last) && last > '0') {
        nh = last - '0';
        return g.substr(0, g.size() - 1);
    }
    return g;
}

std::string Lemme::pos()
{
    if (_pos.empty() && !_renvoi.empty()) {
        Lemme *lr = _lemCore->lemme(_renvoi);
        if (lr != nullptr) return lr->pos();
    }
    return _pos;
}

std::vector<Radical *> Lemme::radical(int r)
{
    auto it = _radicaux.find(r);
    return (it != _radicaux.end()) ? it->second : std::vector<Radical *>();
}

bool Lemme::renvoi() { return contains(_indMorph, "cf. "); }

std::string Lemme::traduction(const std::string &l)
{
    if (l.size() == 2) {
        if (_traduction.count(l))    return _traduction.at(l);
        if (_traduction.count("fr")) return _traduction.at("fr");
        if (_traduction.count("en")) return _traduction.at("en");
        return "non traduit / Translation not available.";
    }
    auto tryLang = [&](const std::string &ll) -> std::string {
        if (_traduction.count(ll)) return _traduction.at(ll);
        return "";
    };
    std::string r;
    if (l.size() >= 2) r = tryLang(l.substr(0,2));
    if (r.empty() && l.size() >= 5) r = tryLang(l.substr(3,2));
    if (r.empty() && l.size() >= 8) r = tryLang(l.substr(6,2));
    return r.empty() ? "non traduit / Translation not available." : r;
}

void Lemme::setHyphen(const std::string &h) { _hyphen = h; }
std::string Lemme::getHyphen()              { return _hyphen; }
