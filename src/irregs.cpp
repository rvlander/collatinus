/*    irregs.cpp  */

#include "irregs.h"
#include "lemCore.h"
#include "lemme.h"
#include "modele.h"

Irreg::Irreg(const std::string &l, LemCore *lemmat)
{
    _lemmat = lemmat;
    std::vector<std::string> ecl = split(l, ':');
    _grq = ecl.at(0);
    if (endsWith(_grq, "*")) {
        _grq.pop_back();
        _exclusif = true;
    } else {
        _exclusif = false;
    }
    _gr = Ch::atone(_grq);
    _lemme = _lemmat->lemme(ecl.at(1));
    _morphos = Modele::listeI(ecl.at(2));
}

bool Irreg::exclusif() { return _exclusif; }
std::string Irreg::gr()  { return _gr; }
std::string Irreg::grq() { return _grq; }
Lemme *Irreg::lemme()    { return _lemme; }
std::vector<int> Irreg::morphos() { return _morphos; }
