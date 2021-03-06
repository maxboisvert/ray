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

/* TODO: find the memory leak in this file -- during the selection of optimal read markers, the memory goes up ? */

#include <assembler/SequencesIndexer.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <core/Parameters.h>
#include <assembler/Loader.h>
#include <core/common_functions.h>
#include <communication/Message.h>
#include <memory/malloc_types.h>

void SequencesIndexer::attachReads(ArrayOfReads*m_myReads,
				RingAllocator*m_outboxAllocator,
				StaticVector*m_outbox,
				int*m_mode,
				int m_wordSize,
				int m_size,
				int m_rank
			){
	if(!m_initiatedIterator){
		m_theSequenceId=0;

		m_activeWorkerIterator.constructor(&m_activeWorkers);
		m_initiatedIterator=true;
		m_maximumAliveWorkers=30000;
	}

	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);


	if(!m_virtualCommunicator->isReady()){
		return;
	}

	if(m_activeWorkerIterator.hasNext()){
		uint64_t workerId=m_activeWorkerIterator.next()->getKey();
		#ifdef ASSERT
		assert(m_aliveWorkers.find(workerId,false)!=NULL);
		assert(!m_aliveWorkers.find(workerId,false)->getValue()->isDone());
		#endif
		m_virtualCommunicator->resetLocalPushedMessageStatus();

		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers.find(workerId,false)->getValue()->isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_aliveWorkers.find(workerId,false)->getValue()->work();
		}

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers.find(workerId,false)->getValue()->isDone()){
			m_workersDone.push_back(workerId);
		}
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus()&&m_activeWorkers.size()==0){
			// there is at least one worker to start
			// AND
			// the number of alive workers is below the maximum
			if(m_theSequenceId<(int)m_myReads->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){
				if(m_theSequenceId%10000==0){
					printf("Rank %i is selecting optimal read markers [%i/%i]\n",m_rank,m_theSequenceId+1,(int)m_myReads->size());
					fflush(stdout);
					if(m_parameters->showMemoryUsage())
						showMemoryUsage(m_rank);
				}

				#ifdef ASSERT
				if(m_theSequenceId==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif
				char sequence[4000];
				#ifdef ASSERT
				assert(m_theSequenceId<(int)m_myReads->size());
				#endif

				m_myReads->at(m_theSequenceId)->getSeq(sequence,m_parameters->getColorSpaceMode(),false);

				bool flag;
				m_aliveWorkers.insert(m_theSequenceId,&m_workAllocator,&flag)->getValue()->constructor(m_theSequenceId,sequence,m_parameters,m_outboxAllocator,m_virtualCommunicator,
					m_theSequenceId,m_myReads,&m_workAllocator);
				m_activeWorkers.insert(m_theSequenceId,&m_workAllocator,&flag);
				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}

				m_theSequenceId++;
			}else{
				m_virtualCommunicator->forceFlush();
			}
		}

		m_activeWorkerIterator.constructor(&m_activeWorkers);
	}

	#ifdef ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorkers);
	#endif

	if((int)m_myReads->size()==m_completedJobs){
		printf("Rank %i is selecting optimal read markers [%i/%i] (completed)\n",m_rank,(int)m_myReads->size(),(int)m_myReads->size());
		fflush(stdout);
		printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_rank,m_maximumWorkers,m_maximumAliveWorkers);
		fflush(stdout);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
		m_outbox->push_back(aMessage);

		m_virtualCommunicator->printStatistics();

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
		}

		#ifdef ASSERT
		assert(m_aliveWorkers.size()==0);
		assert(m_activeWorkers.size()==0);
		#endif

		int freed=m_workAllocator.getNumberOfChunks()*m_workAllocator.getChunkSize();
		m_workAllocator.clear();

		if(m_parameters->showMemoryUsage()){
			cout<<"Rank "<<m_parameters->getRank()<<": Freeing unused assembler memory: "<<freed/1024<<" KiB freed"<<endl;
			showMemoryUsage(m_rank);
		}
	}
}

void SequencesIndexer::constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,VirtualCommunicator*vc){
	m_parameters=parameters;
	int chunkSize=4194304; // 4 MiB
	m_allocator.constructor(chunkSize,RAY_MALLOC_TYPE_OPTIMAL_READ_MARKERS,
		m_parameters->showMemoryAllocations());

	m_workAllocator.constructor(chunkSize,RAY_MALLOC_TYPE_OPTIMAL_READ_MARKER_WORKERS,
		m_parameters->showMemoryAllocations());

	m_initiatedIterator=false;
	m_rank=parameters->getRank();
	m_size=parameters->getSize();
	m_pendingMessages=0;

	m_completedJobs=0;
	m_maximumWorkers=0;
	m_theSequenceId=0;
	m_virtualCommunicator=vc;
}

void SequencesIndexer::setReadiness(){
	m_pendingMessages--;
}

MyAllocator*SequencesIndexer::getAllocator(){
	return &m_allocator;
}

void SequencesIndexer::updateStates(){
	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		uint64_t workerId=m_workersDone[i];
		#ifdef ASSERT
		assert(m_activeWorkers.find(workerId,false)!=NULL);
		assert(m_aliveWorkers.find(workerId,false)!=NULL);
		#endif
		m_activeWorkers.remove(workerId,true,&m_workAllocator);
		m_aliveWorkers.remove(workerId,true,&m_workAllocator);
		m_completedJobs++;
	}
	m_workersDone.clear();

	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		uint64_t workerId=m_waitingWorkers[i];
		#ifdef ASSERT
		assert(m_activeWorkers.find(workerId,false)!=NULL);
		#endif
		m_activeWorkers.remove(workerId,true,&m_workAllocator);
	}
	m_waitingWorkers.clear();

	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		bool flag;
		uint64_t workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId,&m_workAllocator,&flag);
	}
	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}
