#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include "Graph.h"
#include <openssl/sha.h>

using namespace std;


Vertex::Vertex(string method) {
	methodName = method;
	sensitive = false;
}

Vertex::Vertex(string method, bool sensitive) {
	methodName = method;
	this->sensitive = sensitive;
}

Vertex::Vertex(){methodName = "";sensitive=false;}

string Vertex::getMethodName() {return methodName;}

bool Vertex::isSensitive() {return sensitive;}

Edge::Edge(Vertex org, Vertex dest) {
	origin = org;
	destination = dest;
}

Vertex Edge::getOrigin() {return origin;}
Vertex Edge::getDestination() {return destination;}


Graph::Graph() {}

void Graph::insert(Vertex v) {
	vertices.push_back(v);
}

bool Graph::contains(Vertex v){
	return (find(vertices.begin(), vertices.end(), v) != vertices.end());
}

int Graph::addEdge(Vertex origin, Vertex destination) {
	Edge newEdge(origin, destination);
	if(find(vertices.begin(), vertices.end(), origin) == vertices.end()) {
		vertices.push_back(origin);
	} else if(origin.isSensitive()) {
		int index = find(vertices.begin(), vertices.end(), origin) - vertices.begin();
		vertices[index] = origin;
	}
	if(find(vertices.begin(), vertices.end(), destination) == vertices.end()) {
		vertices.push_back(destination);
	} else if(destination.isSensitive()) {
		int index = find(vertices.begin(), vertices.end(), destination) - vertices.begin();
		vertices[index] = destination;
	}
	if(find(edges.begin(), edges.end(), newEdge) == edges.end())
		edges.push_back(newEdge);
}

void Graph::addRegisteredVertex(Vertex v) {
	registeredVertices.push_back(v);
}

vector<Vertex> Graph::getCallees(Vertex v) {
	vector<Vertex> result;
	for(int i = 0; i < edges.size(); i++) {
		if(edges[i].getOrigin() == v) {
			result.push_back(edges[i].getDestination());
		}
	}
	return result;
}

vector<Vertex> Graph::getCallers(Vertex v) {
	vector<Vertex> result;
	for(int i = 0; i < edges.size(); i++) {
		if(edges[i].getDestination() == v) {
			result.push_back(edges[i].getOrigin());
		}
	}
	return result;
}

Vertex Graph::getFirstNode() {
	for(int i = 0; i < vertices.size(); i++) {
		if(getCallers(vertices[i]).size() == 0) {
			return vertices[i];
		}
	}
}

vector<Vertex> Graph::getLastNodes() {
	vector<Vertex> result;
	for(int i = 0; i < vertices.size(); i++) {
		if(getCallees(vertices[i]).size() == 0) {
			result.push_back(vertices[i]);
		}
	}
	return result;
}

vector<Vertex> Graph::getSensitiveNodes() {
	vector<Vertex> result;
	for(Vertex &v : vertices) {
		if(v.isSensitive()) {
			result.push_back(v);
		}
	}
	return result;
}

void rewriteStackAnalysis(string inputFile, string outputFile, string edges, int numVertices, int numEdges) {
	ifstream filein(inputFile); //File to read from
	ofstream fileout(outputFile); //Temporary file
	if(!filein || !fileout){
		cout << "Error opening files!" << endl;
		return;
	}

	string graphTextReplace = "GRAPH_TEXT_PLACEHOLDER";
	string vertReplace = "VERT_COUNT_PLACEHOLDER";
	string lineReplace = "LINE_COUNT_PLACEHOLDER";

	string strTemp;
	size_t findPos;
	while(getline(filein, strTemp)) {
		if((findPos = strTemp.find(graphTextReplace)) != string::npos) {
			strTemp.replace(findPos, graphTextReplace.length(), "\"" + edges + "\"");
		}
		else if((findPos = strTemp.find(vertReplace)) != string::npos) {
			strTemp.replace(findPos, graphTextReplace.length(), to_string(numVertices));
		}
		else if((findPos = strTemp.find(lineReplace)) != string::npos) {
			strTemp.replace(findPos, graphTextReplace.length(), to_string(numEdges));
		}
		strTemp += "\n";
		fileout << strTemp;
	}
}

void Graph::writeGraphFile(string inputFile, string outputFile) {
	vector<Vertex> paths = getPathsToSensitiveNodes();

	vector<Vertex> verticesOnPath;
	vector<Edge> edgesOnPath;
	for(Edge &e : edges) {
		// Only add edges that are contained in a path to a sensitive function to get a
		// smaller adjacency matrix in the stack analysis
		if(find(paths.begin(), paths.end(), e.getDestination()) != paths.end()
				&& find(paths.begin(), paths.end(), e.getOrigin()) != paths.end()) {
					edgesOnPath.push_back(e);
					if( find(verticesOnPath.begin(), verticesOnPath.end(), e.getOrigin()) == verticesOnPath.end())
						verticesOnPath.push_back(e.getOrigin());
					if( find(verticesOnPath.begin(), verticesOnPath.end(), e.getDestination()) == verticesOnPath.end())
						verticesOnPath.push_back(e.getDestination());
		}
	}
	sort(edgesOnPath.begin(), edgesOnPath.end(), [](Edge e1, Edge e2)->bool{
		return e1.getOrigin().str() < e2.getOrigin().str();
	});
	stringstream ss;
	for(Edge &e : edgesOnPath) {
		cout << e.str() << endl;
		ss << e.str() << "\\n";
	}
	string edges = ss.str();

	// Write checksum to file
	rewriteStackAnalysis(inputFile, outputFile, edges, verticesOnPath.size(), edgesOnPath.size());
	return;
}

void Graph::writeStatsFile() {
	ofstream fileout("stats.json");
	if (!fileout) {
		cout << "Error opening stats file" << endl;
		return;
	}

	fileout << "{\n  \"vertices\": [";
	// loop through vertices
	bool isFirst = true;
	for (Vertex &v : registeredVertices) {
		if (isFirst) {
			isFirst = false;
		}
		else {
			fileout << ", ";
		}
		fileout << "\"" + v.str() + "\"";
	}

	fileout << "],\n  \"sensitiveNodes\": [";
	// loop through sensitive nodes
	isFirst = true;
	for (Vertex &v : getSensitiveNodes()) {
		if (isFirst) {
			isFirst = false;
		}
		else {
			fileout << ", ";
		}
		fileout << "\"" + v.str() + "\"";
	}

	fileout << "]\n}";

}

vector<Vertex> Graph::getPathsToSensitiveNodes() {
	vector<Vertex> pathToSensitiveFunctions = getSensitiveNodes();
	int size = pathToSensitiveFunctions.size();
	for(int i = 0; i < size; i++) {
		vector<Vertex> newDominators = getCallers(pathToSensitiveFunctions[i]);
		for(auto it = newDominators.begin() ; it != newDominators.end() ; ++it) {
			if(find(pathToSensitiveFunctions.begin(), pathToSensitiveFunctions.end(), *it)
					== pathToSensitiveFunctions.end()) {
				pathToSensitiveFunctions.push_back(*it);
				size++;
			}
		}
	}
	return pathToSensitiveFunctions;
}
