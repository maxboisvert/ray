/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _SeedWorker
#define _SeedWorker

#include <stdint.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <communication/VirtualCommunicator.h>
#include <vector>
using namespace std;

/*
 * Given a Kmer, SeedWorker determines if it spawns a seed.
 * If yes, it computes the seed.
 */
class SeedWorker{
	map<Kmer,int> m_cache;
	uint64_t m_workerIdentifier;
	bool m_finished;
	Kmer m_SEEDING_currentChildVertex;
	bool m_SEEDING_InedgesReceived;
	Kmer m_SEEDING_currentParentVertex;
	bool m_SEEDING_ingoingEdgesDone;
	int m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage;
	Kmer m_SEEDING_currentVertex;
	int m_SEEDING_numberOfIngoingEdges;
	bool m_SEEDING_vertexCoverageRequested;
	bool m_SEEDING_edgesReceived;
	bool m_SEEDING_testInitiated;
	int m_SEEDING_ingoingEdgeIndex;
	int m_SEEDING_outgoingEdgeIndex;
	vector<Kmer> m_SEEDING_receivedIngoingEdges;
	vector<int> m_ingoingCoverages;
	vector<int> m_outgoingCoverages;
	Parameters*m_parameters;
	bool m_SEEDING_vertexCoverageReceived;
	vector<Kmer> m_SEEDING_receivedOutgoingEdges;
	int m_SEEDING_numberOfOutgoingEdges;
	int m_SEEDING_numberOfIngoingEdgesWithSeedCoverage;
	bool m_SEEDING_outgoingEdgesDone;
	bool m_SEEDING_InedgesRequested;
	bool m_outgoingEdgesReceived;
	bool m_SEEDING_edgesRequested;
	bool m_ingoingEdgesReceived;
	int m_wordSize;

	vector<Kmer> m_SEEDING_seed;
	vector<int> m_coverages;
	bool m_SEEDING_firstVertexParentTestDone;	
	set<Kmer> m_SEEDING_vertices;
	Kmer m_SEEDING_first;
	bool m_SEEDING_firstVertexTestDone;
	int m_SEEDING_numberOfSeedCoverageCandidates;
	int m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	int m_SEEDING_receivedVertexCoverage;
	bool m_SEEDING_1_1_test_result;
	int getRank();
	int getSize();
	bool m_SEEDING_1_1_test_done;
	VirtualCommunicator*m_virtualCommunicator;
public:
	void constructor(Kmer*vertex,Parameters*parameters,RingAllocator*outboxAllocator,
		VirtualCommunicator*vc,uint64_t workerId);
	bool isDone();
	vector<Kmer>*getSeed();
	void do_1_1_test();
	void work();
	uint64_t getWorkerId();
};

#endif
