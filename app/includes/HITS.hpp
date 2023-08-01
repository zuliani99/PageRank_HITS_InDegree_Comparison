#include "Graph.hpp"

class HITS {
	public: 
		HITS(std::vector<int> top_k, std::string ds_path){
			this->top_k = top_k;
			this->graph = Graph(ds_path);
			// std::cout << "Number of nodes " << this->graph.nodes << "\n";
			// std::cout << "Number of edges " << this->graph.edges << "\n";
			// std::cout << "Min node " << this->graph.min_node << "\n";
			// std::cout << "Max node " << this->graph.max_node << "\n";
			// std::cout << "Creating the adjacency matrix L and its transpose.." << "\n";
			this->create_L_and_L_t();
			// std::cout << "Initializing a_k and h_k.." << "\n";
			this->initialize_ak_hk();
			// this->print_authority();
			// this->print_hub();
		}

		// Vector containing the values of k for which the top-k is computed
		std::vector<int> top_k;

		// Vector of authority scores at time k
    	// std::vector<double> a_k;

    	// Vector of hub scores at time k
    	// std::vector<double> h_k;

		// Map: NodeID <-> authority score
		std::unordered_map<int, double> HITS_authority;

		// Map: NodeID <-> hub score
		std::unordered_map<int, double> HITS_hub;

		// Vector containing the final top-k authority scores
		top_k_results<double> authority_topk;

		// Vector containing the final top-k hub scores
		top_k_results<double> hub_topk;
		
		// Number of steps for convergence
		int steps = 0;

		// Elapsed time for computation
		Duration elapsed;
		
		void compute_L();
		void compute_L_t();
		void create_L_and_L_t();
		void initialize_ak_hk();
		void compute();
		void get_topk_authority();
		void get_topk_hub();
		void print_authority();
		void print_hub();
		void print_topk_authority();
		void print_topk_hub();
		void print_stats();

	private:
		// Stores the graph for which the HITS is computed
		Graph graph;

		// Stores the adjacency matrix L storing the column position
    	unsigned int* L_ptr;

		// Stores the starting point of the new line of L
    	std::vector<unsigned int> row_ptr_L;

		// Stores the empty line in L
    	std::vector<unsigned int> row_ptr_not_empty_L;

		// Stores the transpose of the adjacency matrix L storing the column position
    	unsigned int* L_t_ptr;

    	// Stores the starting point of the new line of L_t
    	std::vector<unsigned int> row_ptr_L_t;

    	// Stores the empty line in L_t
    	std::vector<unsigned int> row_ptr_not_empty_L_t;

		bool converge_authority(std::unordered_map<int, double> &temp);
		bool converge_hub(std::unordered_map<int, double> &temp);
		void normalize(std::unordered_map<int, double> &ak, std::unordered_map<int, double> &hk);
};

// Function that computes the adjacency matrix L
void HITS::compute_L(){
        // Allocating the right amount of space in permanent memory to save the edges of graph as a set of pairs
        this->L_ptr = (unsigned int*)mmap(NULL, this->graph.edges * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

        // Checking the allocation
        if (L_ptr == MAP_FAILED){
            throw std::runtime_error("Mapping failed\n");
        }

        this->row_ptr_L.push_back(0);
        this->row_ptr_not_empty_L.push_back(this->graph.np_pointer[0].first - this->graph.min_node);

        // Computing L
        for (unsigned int i = 0; i < this->graph.edges; i++){
            if (i > 0 && this->graph.np_pointer[i - 1].first != this->graph.np_pointer[i].first){
                row_ptr_L.push_back(i);
                this->row_ptr_not_empty_L.push_back(this->graph.np_pointer[i].first - this->graph.min_node);
            }
            this->L_ptr[i] = this->graph.np_pointer[i].second;
        }
        this->row_ptr_L.push_back(this->graph.edges);
    }

// Function that computes the transpose matrix of L
void HITS::compute_L_t(){
	// Allocating the right amount of space in permanent memory to save the edges of graph as a set of pairs
	this->L_t_ptr = (unsigned int*)mmap(NULL, this->graph.edges * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// Checking the allocation
	if (L_t_ptr == MAP_FAILED){
		throw std::runtime_error("Mapping failed\n");
	}

	this->row_ptr_L_t.push_back(0);
	this->row_ptr_not_empty_L_t.push_back(this->graph.np_pointer[0].second - this->graph.min_node);
	
	// Computing L_t
	for (unsigned int i = 0; i < this->graph.edges; i++){
		if (i > 0 && this->graph.np_pointer[i - 1].second != this->graph.np_pointer[i].second){
			row_ptr_L_t.push_back(i);
			this->row_ptr_not_empty_L_t.push_back(this->graph.np_pointer[i].second - this->graph.min_node);
		}
		this->L_t_ptr[i] = this->graph.np_pointer[i].first;
	}
	this->row_ptr_L_t.push_back(this->graph.edges);
}

// Function that creates L and L_t
void HITS::create_L_and_L_t(){
	// Sorting ptr with a stable sort w.r.t. the first column, in this way we are able to compute the affinity matrix
	std::stable_sort(this->graph.np_pointer, this->graph.np_pointer + this->graph.edges, compareByFirstIncreasing);
	this->compute_L();

	// Sorting again w.r.t. second element 
	std::stable_sort(this->graph.np_pointer, this->graph.np_pointer + this->graph.edges, compareBySecondIncreasing);
	this->compute_L_t();

	// Free memory 
	if (munmap(this->graph.np_pointer, this->graph.edges) != 0){
		throw std::runtime_error("Free memory failed\n");
	}
}

// Function that initializes authority and hub vectors
void HITS::initialize_ak_hk(){
	for (int i = 0; i < this->graph.nodes; i++) this->HITS_authority[i] = 1.;
	for (int i = 0; i < this->graph.nodes; i++) this->HITS_hub[i] = 1.;
}

// Function that computes autority and hub vectors
void HITS::compute(){
    this->steps = 0;

    std::unordered_map<int, double> temp_HITS_authority;
	for (int i = 0; i < this->graph.nodes; i++) temp_HITS_authority[i] = 1.;
	std::unordered_map<int, double> temp_HITS_hub;
	for (int i = 0; i < this->graph.nodes; i++) temp_HITS_hub[i] = 1.;

    bool first_cicle = true;
	auto start = now();

    do {
        this->steps++;

        // Computing hub scores at time 1
        unsigned int tmp_pos_row = 0;
        unsigned int next_starting_row = this->row_ptr_L[1];
        for (unsigned int i = 0; i < this->graph.edges; i++){
            if (i == next_starting_row){
                tmp_pos_row++;
                next_starting_row = this->row_ptr_L[tmp_pos_row + 1];
            }
            if (first_cicle){
                temp_HITS_hub[this->row_ptr_not_empty_L[tmp_pos_row]] += 1;
            }
            else{
                temp_HITS_hub[this->row_ptr_not_empty_L[tmp_pos_row]] += this->HITS_hub[this->L_ptr[i]];
            }
        }

    	// Computing authority scores at time 1
        tmp_pos_row = 0;
        next_starting_row = this->row_ptr_L_t[1];
        for (unsigned int i = 0; i < this->graph.edges; i++){
            if (i == next_starting_row){
                tmp_pos_row++;
                next_starting_row = this->row_ptr_L_t[tmp_pos_row + 1];
            }
            if (first_cicle){
                temp_HITS_authority[this->row_ptr_not_empty_L_t[tmp_pos_row]] += 1;
            }
            else{
                temp_HITS_authority[this->row_ptr_not_empty_L_t[tmp_pos_row]] += this->HITS_authority[this->L_t_ptr[i]];
            }
        }
        if (first_cicle){
            first_cicle = false;
        }
        this->normalize(temp_HITS_authority, temp_HITS_hub);
    } while (this->converge_authority(temp_HITS_authority) and this->converge_hub(temp_HITS_hub));
	this->elapsed = now() - start;
}

// Function that establishes whether the execution of the HITS algorithm should continue or not 
bool HITS::converge_authority(std::unordered_map<int, double> &temp){
	double distance = 0.;
	for (int i = 0; i < temp.size(); i++) 
		distance += std::abs(this->HITS_authority[i] - temp[i]);

	this->HITS_authority = temp;

	temp.clear();
	for (int i = 0; i < this->graph.nodes; i++) temp[i] = 1.;

	return distance > std::pow(10, -6);
}

// Function that establishes whether the execution of the HITS algorithm should continue or not 
bool HITS::converge_hub(std::unordered_map<int, double> &temp){
	double distance = 0.;
	for (int i = 0; i < temp.size(); i++) 
		distance += std::abs(this->HITS_hub[i] - temp[i]);

	this->HITS_hub = temp;

	temp.clear();
	for (int i = 0; i < this->graph.nodes; i++) temp[i] = 1.;

	return distance > std::pow(10, -6);
}

// Function that normalizes the vectors in order to obtain a probability distribution
void HITS::normalize(std::unordered_map<int, double> &ak, std::unordered_map<int, double> &hk){
	double sum_a_k = 0.0;
	double sum_h_k = 0.0;
	
	for (int i = 0; i < this->graph.nodes; i++){
		sum_a_k += ak[i];
		sum_h_k += hk[i];
	}

	for (int i = 0; i < this->graph.nodes; i++){
		ak[i] = ak[i] / sum_a_k;
		hk[i] = hk[i] / sum_h_k;
	}
}

// Function that gets the top-k nodes w.r.t. the authority score
void HITS::get_topk_authority() {
	std::vector<std::pair<int, double>> HITS_authority_vec_pairs(this->HITS_authority.begin(), this->HITS_authority.end());
	std::sort(HITS_authority_vec_pairs.begin(), HITS_authority_vec_pairs.end(), compareBySecondDecreasing<int, double>);

	for(int k : this->top_k) {
		std::vector<std::pair<int, double>> final_top_k(HITS_authority_vec_pairs.begin(), HITS_authority_vec_pairs.begin() + k);
		this->authority_topk[k] = final_top_k;
	}
}

// Function that gets the top-k nodes w.r.t. the hub score
void HITS::get_topk_hub() {
	std::vector<std::pair<int, double>> HITS_hub_vec_pairs(this->HITS_hub.begin(), this->HITS_hub.end());
	std::sort(HITS_hub_vec_pairs.begin(), HITS_hub_vec_pairs.end(), compareBySecondDecreasing<int, double>);

	for(int k : this->top_k) {
		std::vector<std::pair<int, double>> final_top_k(HITS_hub_vec_pairs.begin(), HITS_hub_vec_pairs.begin() + k);
		this->hub_topk[k] = final_top_k;
	}
}

// Function that prints the content of the authority vector
void HITS::print_authority(){
	std::cout << "Values of a_k = [";
	for (int i = 0; i < this->HITS_authority.size(); i++){
		std::cout << HITS_authority[i] << ",";
	}
	std::cout << "]\n";
}

// Function that prints the content of the hub vector
void HITS::print_hub(){
	std::cout << "Values of h_k = [";
	for (int i = 0; i < this->HITS_hub.size(); i++){
		std::cout << HITS_hub[i] << ",";
	}
	std::cout << "]\n";
}

// Function that prints the authority scores for the top-k nodes
void HITS::print_topk_authority() {
	std::cout << "Authority scores" << std::endl;
	for (auto p : this->authority_topk){
		std::cout << "Top " << p.first << std::endl;
		int i = 1;
		for (auto node : p.second) {
			std::cout << i << ") Node ID: " << node.first << " - Authorty score: " << node.second << std::endl;
			i++;
		}
	}
}

// Function that prints the hub scores for the top-k nodes
void HITS::print_topk_hub() {
	std::cout << "Hub scores" << std::endl;
	for (auto p : this->hub_topk){
		std::cout << "Top " << p.first << std::endl;
		int i = 1;
		for (auto node : p.second) {
			std::cout << i << ") Node ID: " << node.first << " - Hub score: " << node.second << std::endl;
			i++;
		}
	}
}

// Function that prints the execution time and the number of steps taken by the HITS algorithm to converge
void HITS::print_stats() {
	std::cout << "Elapsed: " << this->elapsed.count() << " ms \t Steps: "<< this->steps << std::endl;
}