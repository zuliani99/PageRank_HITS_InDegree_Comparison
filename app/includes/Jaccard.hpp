#ifndef _JACCARD_H
#define _JACCARD_H

#include "./Utils.hpp"

// This class provides the implementation of the Jaccard coefficient, which is used to compare the results obtained by the algorithms.
class JaccardCoefficient {
	public: 
		// JaccardCoefficient constructor.
		JaccardCoefficient(std::vector<unsigned int> topk, top_k_results &ID_topk, top_k_results &PR_topk, top_k_results &HITS_authority_topk, top_k_results &HITS_hub_topk) {
			this->topk = topk;

			// retrieving the IDs of the top-k nodes for each top-k results for all the algorithms
			this->nodes_ID_topk = this->get_first_element_vector(ID_topk);
			this->nodes_PR_topk = this->get_first_element_vector(PR_topk);
			this->nodes_HITS_authority_topk = this->get_first_element_vector(HITS_authority_topk);
			this->nodes_HITS_hub_topk = this->get_first_element_vector(HITS_hub_topk);			
		}

		// Public functions declaration

		void obtain_results();
		void print_results();
		void save_results(std::fstream &stream_jaccard, std::string &ds);

	private:
		std::map<unsigned int, std::vector<unsigned int>> nodes_ID_topk;
		std::map<unsigned int, std::vector<unsigned int>> nodes_PR_topk;
		std::map<unsigned int, std::vector<unsigned int>> nodes_HITS_authority_topk;
		std::map<unsigned int, std::vector<unsigned int>> nodes_HITS_hub_topk;
		std::vector<unsigned int> topk;

		// 		<k		,	<'Algo1_Algo2', res		>>
		std::map<unsigned int, std::vector<std::pair<std::string, double>>> jaccard_results;

		// Private function declaration

		std::map<unsigned int, std::vector<unsigned int>> get_first_element_vector(top_k_results &topk_vector);
		std::vector<unsigned int> intersection(std::vector<unsigned int> &v1, std::vector<unsigned int> &v2);
		double jaccard_coefficient(std::vector<unsigned int> &v1, std::vector<unsigned int> &v2);
};

// Function that retrieves the IDs of the top-k nodes for each top-k results.
std::map<unsigned int, std::vector<unsigned int>> JaccardCoefficient::get_first_element_vector(top_k_results &topk_vector) {

	// for each value of k, we store the IDs of the top-k nodes
	std::map<unsigned int, std::vector<unsigned int>> jaccard_results;

	for(unsigned int k : this->topk) {

		// initializing the IDs vector with size k
		std::vector<unsigned int> firstElements(k); 

		std::transform(topk_vector[k].begin(), topk_vector[k].end(), firstElements.begin(),
						[](const std::pair<unsigned int, double> &pair) {
							return pair.first;
						});

		// setting the array on the unordered map at k
		jaccard_results[k] = firstElements; 
	}
	return jaccard_results;
}

// Function that computes the intersection between two given vectors.
std::vector<unsigned int> JaccardCoefficient::intersection(std::vector<unsigned int> &v1, std::vector<unsigned int> &v2) {
    std::vector<unsigned int> intersect;

	std::stable_sort(v1.begin(), v1.end());
	std::stable_sort(v2.begin(), v2.end());
 
    // finding the intersection of the two sets
    std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), std::inserter(intersect, intersect.begin()));
 
    return intersect;
}

// Function that returns the Jaccard index of two sets.
double JaccardCoefficient::jaccard_coefficient(std::vector<unsigned int> &v1, std::vector<unsigned int> &v2) {

    // getting the cardinality of the intersection 
    double size_in = this->intersection(v1, v2).size();
 
    // return the jaccard coefficient 
    return size_in / (v1.size() + v2.size() - size_in);
}

// Function that computes the actual Jaccard coefficient between all the possible pairs of algorithm.
void JaccardCoefficient::obtain_results() {
	for (unsigned int k : this->topk) {
		std::vector<std::pair<std::string, double>> temp_res;

		// ID_HITS
		temp_res.push_back(std::make_pair<std::string, double>("InDegree VS HITS (authority)", this->jaccard_coefficient(this->nodes_ID_topk[k], this->nodes_HITS_authority_topk[k])));
		temp_res.push_back(std::make_pair<std::string, double>("InDegree VS HITS (hub)", this->jaccard_coefficient(this->nodes_ID_topk[k], this->nodes_HITS_hub_topk[k])));
	
		// ID_PR
		temp_res.push_back(std::make_pair<std::string, double>("InDegree VS PageRank", this->jaccard_coefficient(this->nodes_ID_topk[k], this->nodes_PR_topk[k])));

		// PR_HITS
		temp_res.push_back(std::make_pair<std::string, double>("PageRank VS HITS (authority)", this->jaccard_coefficient(this->nodes_PR_topk[k], this->nodes_HITS_authority_topk[k])));
		temp_res.push_back(std::make_pair<std::string, double>("PageRank VS HITS (hub)", this->jaccard_coefficient(this->nodes_PR_topk[k], this->nodes_HITS_hub_topk[k])));

		// AUT_HUB
		temp_res.push_back(std::make_pair<std::string, double>("HITS (authority) VS HITS (hub)", this->jaccard_coefficient(this->nodes_HITS_authority_topk[k], this->nodes_HITS_hub_topk[k])));

		this->jaccard_results[k] = temp_res;
	}
}

// Function that prints the results.
void JaccardCoefficient::print_results() {
	for(auto pair1 : this->jaccard_results) {
		std::cout << "TOP " << pair1.first;
		for(auto result : pair1.second)
			std::cout << "\t" << result.first << ": " << result.second << std::endl; 
		std::cout << std::endl;
	}
}

// Function that saves the jaccard coefficients in a .csv file.
void JaccardCoefficient::save_results(std::fstream &stream_jaccard, std::string &ds) {
	for(auto pair1 : this->jaccard_results) {
		stream_jaccard << ds << "," << pair1.first;
		for(auto result : pair1.second) stream_jaccard << "," << result.second; 
		stream_jaccard << '\n';
	}
}

#endif