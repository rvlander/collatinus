/*      irregs.h
 *
 *  This file is part of COLLATINUS.
 *
 *  COLLATINUS is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  COLLATINVS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with COLLATINUS; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * © Yves Ouvrard, 2009 - 2016
 */

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
