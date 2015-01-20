#pragma once

typedef unsigned long ClusterNo;
const unsigned long ClusterSize = 2048;

class PartitionImpl;

class Partition {
public:
	Partition(char *);
	virtual ClusterNo getNumOfClusters() const; //returns number of clusters for partition

	virtual int readCluster(ClusterNo, char *buffer); //reads cluster to memory buffer
	virtual int writeCluster(ClusterNo, const char *buffer); //writes from memory buffer to cluster

	virtual ~Partition();
private:
	PartitionImpl *myImpl;
};
