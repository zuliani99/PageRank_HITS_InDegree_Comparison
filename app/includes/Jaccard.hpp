#ifndef _JACCARD_H
#define _JACCARD_H

#include "./Utils.hpp"

// This class provides the implementation of the Jaccard coefficient, which is used to compare the results obtained by the algorithms
class JaccardCoefficient {
	public: 
		JaccardCoefficient(std::vector<int> topk, top_k_results<int> &ID_topk, 
					top_k_results<double> &PR_topk, top_k_results<double> &HITS_authority_topk, top_k_results<double> &HITS_hub_topk) {
			this->topk = topk;

			// Retrieves the IDs of the top_k nodes for each top_k results for all the algorithms
			this->nodes_ID_topk = this->get_first_element_vector<int>(ID_topk);

			this->nodes_PR_topk = this->get_first_element_vector<double>(PR_topk);
			this->nodes_HITS_authority_topk = this->get_first_element_vector<double>(HITS_authority_topk);
			this->nodes_HITS_hub_topk = this->get_first_element_vector<double>(HITS_hub_topk);			
		}

		// Public functions declaration

		void obtain_results();
		void print_results();
		void save_results(std::fstream &stream_jaccard, std::string &ds);

	private:
		std::map<int, std::vector<int>> nodes_ID_topk;
		std::map<int, std::vector<int>> nodes_PR_topk;
		std::map<int, std::vector<int>> nodes_HITS_authority_topk;
		std::map<int, std::vector<int>> nodes_HITS_hub_topk;
		std::vector<int> topk;

		// 		<k		,	<'Algo1_Algo2', res		>>
		std::map<int, std::vector<std::pair<std::string, double>>> jaccard_results;


		// Private functions declaration

		template<typename T>
		std::map<int, std::vector<int>> get_first_element_vector(top_k_results<T> &topk_vector);

		std::vector<int> intersection(std::vector<int> &v1, std::vector<int> &v2);
		double jaccard_coefficient(std::vector<int> &v1, std::vector<int> &v2);
};

// Retrieves the IDs of the top_k nodes for each top_k results
template<typename T>
std::map<int, std::vector<int>> JaccardCoefficient::get_first_element_vector(top_k_results<T> &topk_vector) {

	// for each value of k, we store the IDs of the top-k nodes
	std::map<int, std::vector<int>> jaccard_results;

	for(int k : this->topk) {

		// initialize the IDs vector with size k
		std::vector<int> firstElements(k); 

		std::transform(topk_vector[k].begin(), topk_vector[k].end(), firstElements.begin(),
						[](const std::pair<int, T> &pair) {
							return pair.first;
						});

		// set the array on the k unordered map position
		jaccard_results[k] = firstElements; 
	}

	return jaccard_results;
}

// Computes the intersection between two given vectors
std::vector<int> JaccardCoefficient::intersection(std::vector<int> &v1, std::vector<int> &v2) {
    std::vector<int> intersect;

	std::stable_sort(v1.begin(), v1.end());
	std::stable_sort(v2.begin(), v2.end());
 
    // find the intersection of the two sets
    std::set_intersection(v1.begin(), v1.end(), v2.begin(), v2.end(), 
			std::inserter(intersect, intersect.begin()));
 
    return intersect;
}

// Returns the Jaccard index of two sets
double JaccardCoefficient::jaccard_coefficient(std::vector<int> &v1, std::vector<int> &v2) {

    // get the cardinality of the intersection 
    double size_in = this->intersection(v1, v2).size();
 
    // return the jaccard coefficient 
    return size_in / (v1.size() + v2.size() - size_in);
}

// Computes the actual Jaccard coefficient between all the possible pairs of algorithm
void JaccardCoefficient::obtain_results() {
	for (int k : this->topk) {
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

// Prints the results
void JaccardCoefficient::print_results() {
	for(auto pair1 : this->jaccard_results) {
		std::cout << "TOP " << pair1.first;
		for(auto result : pair1.second)
			std::cout << "\t" << result.first << ": " << result.second << std::endl; 
		std::cout << std::endl;
	}
}

// Saves the jaccard coefficients in a .csv file
void JaccardCoefficient::save_results(std::fstream &stream_jaccard, std::string &ds) {
	for(auto pair1 : this->jaccard_results) {
		stream_jaccard << ds << "," << pair1.first;
		for(auto result : pair1.second) stream_jaccard << "," << result.second; 
		stream_jaccard << '\n';
	}
}

#endif