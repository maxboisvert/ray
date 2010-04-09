/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _OpenAssemblerChooser
#define _OpenAssemblerChooser

#include<Chooser.h> // for IMPOSSIBLE_CHOICE

/**
 * de Bruijn heuristic to choose extension direction in a graph, described in paper (in revision).
 */
class OpenAssemblerChooser{
	int proceedWithCoverages(int a,int b,ExtensionData*m_ed);
public:
	int choose(ExtensionData*m_ed,Chooser*m_c,int m_minimumCoverage,int m_maxCoverage,ChooserData*m_cd);

};

#endif