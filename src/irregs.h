/*      irregs.h  */

#ifndef IRREGS_H
#define IRREGS_H

#include <string>
#include <vector>
#include "ch.h"

class LemCore;
class Lemme;

class Irreg
{
   private:
    bool _exclusif;
    std::string _gr;
    std::string _grq;
    LemCore *_lemmat;
    Lemme *_lemme;
    std::vector<int> _morphos;

   public:
    Irreg(const std::string &l, LemCore *lemmat);
    bool exclusif();
    std::string gr();
    std::string grq();
    Lemme *lemme();
    std::vector<int> morphos();
};

#endif
