#include "Graph.hpp"

// This class provides the implementation of the HITS algorithm.
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

		// Vector containing the values of k for which the top-k is computed.
		std::vector<int> top_k;

		// Vector of authority scores at time k.
    	// std::vector<double> a_k;

    	// Vector of hub scores at time k.
    	// std::vector<double> h_k;

		// Map: NodeID <-> authority score.
		std::unordered_map<int, double> HITS_authority;

		// Map: NodeID <-> hub score.
		std::unordered_map<int, double> HITS_hub;

		// Vector containing the final top-k authority scores.
		top_k_results<double> authority_topk;

		// Vector containing the final top-k hub scores.
		top_k_results<double> hub_topk;
		
		// Number of steps for convergence.
		int steps = 0;

		// Elapsed time for computation.
		Duration elapsed;
		
		void compute_L();
		void compute_L_t();
		void compute_LLt();
		void compute_LtL();
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

		// Pointer to the destination nodes of the adjacency matrix L
    	unsigned int* L_ptr;

		// Stores the index of a new row of L
    	std::vector<unsigned int> row_ptr_L;

		// Stores the empty line in L
    	std::vector<unsigned int> row_ptr_not_empty_L;

		// Pointer to the destination nodes of the transpose od the adjacency matrix L, L_t
    	unsigned int* L_t_ptr;

    	// Stores the index of a new row of L_t
    	std::vector<unsigned int> row_ptr_L_t;

    	// Stores the empty line in L_t
    	std::vector<unsigned int> row_ptr_not_empty_L_t;

    	unsigned int* LLt_ptr;
    	std::vector<unsigned int> row_ptr_LLt;
    	std::vector<unsigned int> row_ptr_not_empty_LLt;
		unsigned int len_LLt;

		unsigned int* LtL_ptr;
    	std::vector<unsigned int> row_ptr_LtL;
    	std::vector<unsigned int> row_ptr_not_empty_LtL;
		unsigned int len_LtL;

		std::string autority_str = "Authority"; 
		std::string hub_str = "Hub"; 

		bool converge(std::unordered_map<int, double> &temp_a, std::unordered_map<int,double> &temp_h);
		void normalize(std::unordered_map<int, double> &ak, std::unordered_map<int, double> &hk);
};

// Function that computes the adjacency matrix L.
void HITS::compute_L(){
	
	// allocating the right amount of space in the permanent memory to save the edges of graph as a set of pairs
	this->L_ptr = (unsigned int*)mmap(NULL, this->graph.edges * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

	// checking the allocation
	if (L_ptr == MAP_FAILED){
		throw std::runtime_error("Mapping failed\n");
	}

	this->row_ptr_L.push_back(0);
	this->row_ptr_not_empty_L.push_back(this->graph.np_pointer[0].first - this->graph.min_node);

	// std::cout << "row_ptr_L = " << row_ptr_L[0] << "\n";
	// std::cout << "row_ptr_not_empty_L = " << row_ptr_not_empty_L[0] << "\n";

	// computing L, the adjacency matrix
	for (unsigned int i = 0; i < this->graph.edges; i++){
		if (i > 0 && this->graph.np_pointer[i - 1].first != this->graph.np_pointer[i].first){
			this->row_ptr_L.push_back(i);
			this->row_ptr_not_empty_L.push_back(this->graph.np_pointer[i].first - this->graph.min_node);
		}
		this->L_ptr[i] = this->graph.np_pointer[i].second;
	}
	this->row_ptr_L.push_back(this->graph.edges);

	// std::cout << "DEBUG" << "\n";
	// for (int i = 0; i<8; i++){
	// 	std::cout << "L_ptr[" << i << "] = " << this->L_ptr[i] << "\n";
	// }

	// for (int i = 0; i<this->row_ptr_L.size(); i++){
	// 	std::cout << "row_ptr_L[" << i << "] = " << this->row_ptr_L[i] << "\n";
	// }

	// for (int i = 0; i<this->row_ptr_not_empty_L.size(); i++){
	// 	std::cout << "row_ptr_not_empty_L[" << i << "] = " << this->row_ptr_not_empty_L[i] << "\n";
	// }
}

// Function that computes the transpose matrix of the adjacency matrix L, L_t.
void HITS::compute_L_t(){

	// allocating the right amount of space in the permanent memory to save the edges of graph as a set of pairs
	this->L_t_ptr = (unsigned int*)mmap(NULL, this->graph.edges * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// checking the allocation
	if (L_t_ptr == MAP_FAILED){
		throw std::runtime_error("Mapping failed\n");
	}

	this->row_ptr_L_t.push_back(0);
	this->row_ptr_not_empty_L_t.push_back(this->graph.np_pointer[0].second - this->graph.min_node);
	
	// computing L_t, the transpose of the adjacency matrix
	for (unsigned int i = 0; i < this->graph.edges; i++){
		if (i > 0 && this->graph.np_pointer[i - 1].second != this->graph.np_pointer[i].second){
			this->row_ptr_L_t.push_back(i);
			this->row_ptr_not_empty_L_t.push_back(this->graph.np_pointer[i].second - this->graph.min_node);
		}
		this->L_t_ptr[i] = this->graph.np_pointer[i].first;
	}
	this->row_ptr_L_t.push_back(this->graph.edges);

	// std::cout << "DEBUG" << "\n";
	// for (int i = 0; i<8; i++){
	// 	std::cout << "L_t_ptr[" << i << "] = " << this->L_t_ptr[i] << "\n";
	// }

	// for (int i = 0; i<this->row_ptr_L_t.size(); i++){
	// 	std::cout << "row_ptr_L_t[" << i << "] = " << this->row_ptr_L_t[i] << "\n";
	// }

	// for (int i = 0; i<this->row_ptr_not_empty_L_t.size(); i++){
	// 	std::cout << "row_ptr_not_empty_L_t[" << i << "] = " << this->row_ptr_not_empty_L_t[i] << "\n";
	// }
}

// Function that computes the matrix multiplication between L and L_t.
void HITS::compute_LLt() {
	unsigned int* LLt_ptr_temp = (unsigned int*)mmap(NULL, this->graph.nodes * this->graph.nodes * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// checking the allocation
	if (LLt_ptr_temp == MAP_FAILED)
		throw std::runtime_error("Mapping failed\n");
	
	int c = 0;

	this->row_ptr_not_empty_LLt = this->row_ptr_not_empty_L;

	for (unsigned int r = 0; r <= this->row_ptr_L.size() - 2; r++){
		this->row_ptr_LLt.push_back(c);

		std::vector<unsigned int> temp_col_idx;
		for(int i = this->row_ptr_L[r]; i < this->row_ptr_L[r + 1]; i ++) 
			temp_col_idx.push_back(this->L_ptr[i]);

		for(int jump = 0; jump <= this->row_ptr_L.size() - 2; jump ++){
			unsigned int temp_LLt = 0;
			for(unsigned int j = this->row_ptr_L[jump]; j < this->row_ptr_L[jump + 1]; j ++) {

				if(std::find(temp_col_idx.begin(), temp_col_idx.end(), this->L_ptr[j]) != temp_col_idx.end())
					temp_LLt += 1;
			}

			if(temp_LLt > 0) {
				LLt_ptr_temp[c] = temp_LLt;
				c += 1;
			}
		}
	}
	// std::cout << "DEBUG: compute_LLt()\nc = " << c << "\n";
	this->len_LLt = c;
	this->LLt_ptr = (unsigned int*)mmap(NULL, this->len_LLt * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// checking the allocation
	if (this->LLt_ptr == MAP_FAILED)
		throw std::runtime_error("Mapping failed\n");

	std::cout << "LLt_ptr = [";
	for(int i = 0; i < this->len_LLt; i++){
		this->LLt_ptr[i] = LLt_ptr_temp[i];
		std::cout << this->LLt_ptr[i] << ", ";
	}
	std::cout << "]\n\n";

	if (munmap(LLt_ptr_temp, this->graph.nodes * this->graph.nodes) != 0)
    	throw std::runtime_error("Free memory failed\n");
}

// Function that computes the matrix multiplication between L_t and L.
void HITS::compute_LtL() {
	unsigned int* LtL_ptr_temp = (unsigned int*)mmap(NULL, this->graph.nodes * this->graph.nodes * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// checking the allocation
	if (LtL_ptr_temp == MAP_FAILED)
		throw std::runtime_error("Mapping failed\n");
	
	int c = 0;

	this->row_ptr_not_empty_LtL = this->row_ptr_not_empty_L_t;

	for (unsigned int r = 0; r <= this->row_ptr_L_t.size() - 2; r++){
		this->row_ptr_LtL.push_back(c);

		std::vector<unsigned int> temp_col_idx;
		for(int i = this->row_ptr_L_t[r]; i < this->row_ptr_L_t[r + 1]; i ++) 
			temp_col_idx.push_back(this->L_t_ptr[i]);

		for(int jump = 0; jump <= this->row_ptr_L_t.size() - 2; jump ++){
			unsigned int temp_LtL = 0;

			for(unsigned int j = this->row_ptr_L_t[jump]; j < this->row_ptr_L_t[jump + 1]; j ++) {
				if(std::find(temp_col_idx.begin(), temp_col_idx.end(), this->L_t_ptr[j]) != temp_col_idx.end())
					temp_LtL += 1;
			}

			if(temp_LtL > 0) {
				LtL_ptr_temp[c] = temp_LtL;
				c += 1;
			}
		}
	}

	// std::cout << "DEBUG: compute_LtL()\nc = " << c << "\n";
	this->len_LtL = c;
	this->LtL_ptr = (unsigned int*)mmap(NULL, this->len_LLt * sizeof(unsigned int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	
	// checking the allocation
	if (this->LtL_ptr == MAP_FAILED)
		throw std::runtime_error("Mapping failed\n");

	std::cout << "LtL_ptr = [";
	for(int i = 0; i < this->len_LtL; i++){
		this->LtL_ptr[i] = LtL_ptr_temp[i];
		std::cout << this->LtL_ptr[i] << ", ";
	}
	std::cout << "]\n\n";

	if (munmap(LtL_ptr_temp, this->graph.nodes * this->graph.nodes) != 0)
    	throw std::runtime_error("Free memory failed\n");
}

// Function that creates L and L_t and that performs the two matrix multiplications.
void HITS::create_L_and_L_t(){

	// sorting the pairs w.r.t. the source node
	std::stable_sort(this->graph.np_pointer, this->graph.np_pointer + this->graph.edges, compareByFirstIncreasing);
	this->compute_L();

	// sorting again w.r.t. the destination node 
	std::stable_sort(this->graph.np_pointer, this->graph.np_pointer + this->graph.edges, compareBySecondIncreasing);
	this->compute_L_t();

	this->compute_LLt();
	this->compute_LtL();

	// freeing the memory 
	this->graph.freeMemory();
}

// Function that initializes authority and hub vectors.
void HITS::initialize_ak_hk(){
	for (int i = 0; i < this->graph.nodes; i++) this->HITS_authority[i] = 1.;
	for (int i = 0; i < this->graph.nodes; i++) this->HITS_hub[i] = 1.;
}

// Function that computes autority and hub vectors.
void HITS::compute(){
    this->steps = 0;

	// auxiliary data structures 
    std::unordered_map<int, double> temp_HITS_authority;
	for (int i = 0; i < this->graph.nodes; i++) temp_HITS_authority[i] = 0.;
	std::unordered_map<int, double> temp_HITS_hub;
	for (int i = 0; i < this->graph.nodes; i++) temp_HITS_hub[i] = 0.;

	auto start = now();

	// repeat until convergence
    do {
		// std::cout << "DEBUG\n step = " << steps << "\n";
        this->steps++;

        // authority score
		// a_k = L^t * L * a_k-1
        unsigned int tmp_pos_row_LtL = 0;
        unsigned int next_starting_row_LtL = this->row_ptr_LtL[1];

		// std::cout << "DEBUG: compute()\n";
		// std::cout << "row_ptr_not_empty_LtL = [";
		// for (int i = 0; i<row_ptr_not_empty_LtL.size(); i++){
		// 	std::cout << row_ptr_not_empty_LtL[i] << ", ";
		// }
		// std::cout << "]\n";

        for (unsigned int i = 0; i < this->len_LtL; i++){
            if (i == next_starting_row_LtL){
                tmp_pos_row_LtL++;
                next_starting_row_LtL = this->row_ptr_LtL[tmp_pos_row_LtL + 1];
            }
            temp_HITS_authority[this->row_ptr_not_empty_LtL[tmp_pos_row_LtL]] += this->LtL_ptr[i] * this->HITS_authority[this->L_ptr[i]];
        }

		// std::cout << "STEP = " << steps << "\nauthority = [";
		// for (int i = 0; i<temp_HITS_authority.size(); i++){
		// 	std::cout << temp_HITS_authority[i] << ", ";
		// }
		// std::cout << "]\n";

		// hub score
    	// h_k = L * L^t * h_k-1
        unsigned int tmp_pos_row_LLt = 0;
        unsigned int next_starting_row_LLt = this->row_ptr_LLt[1];

		// std::cout << "row_ptr_not_empty_LLt = [";
		// for (int i = 0; i<row_ptr_not_empty_LLt.size(); i++){
		// 	std::cout << row_ptr_not_empty_LLt[i] << ", ";
		// }
		// std::cout << "]\n";

        for (unsigned int i = 0; i < this->len_LLt; i++){
            if (i == next_starting_row_LLt){
                tmp_pos_row_LLt ++;
                next_starting_row_LLt = this->row_ptr_LLt[tmp_pos_row_LLt + 1];
            }
            temp_HITS_hub[this->row_ptr_not_empty_LLt[tmp_pos_row_LLt]] += this->LLt_ptr[i] * this->HITS_hub[this->L_t_ptr[i]];
        }

		// std::cout << "hub = [";
		// for (int i = 0; i<temp_HITS_hub.size(); i++){
		// 	std::cout << temp_HITS_hub[i] << ", ";
		// }
		// std::cout << "]\n";

        this->normalize(temp_HITS_authority, temp_HITS_hub);
		// std::cout << "After normalization\n";
		// std::cout << "authority = [";
		// for (int i = 0; i<temp_HITS_authority.size(); i++){
		// 	std::cout << temp_HITS_authority[i] << ", ";
		// }
		// std::cout << "]\n";
		// std::cout << "hub = [";
		// for (int i = 0; i<temp_HITS_hub.size(); i++){
		// 	std::cout << temp_HITS_hub[i] << ", ";
		// }
		// std::cout << "]\n\n";
    } while (this->converge(temp_HITS_authority, temp_HITS_hub));
	
	this->elapsed = now() - start;
}

// Function that establishes whether the execution of the HITS algorithm should continue or not.
bool HITS::converge(std::unordered_map<int, double> &temp_a, std::unordered_map<int,double> &temp_h){
	double distance_a = 0.;
	double distance_h = 0.;

	for (int i = 0; i < temp_a.size(); i++) 
		distance_a += std::abs(this->HITS_authority[i] - temp_a[i]);

	for (int i = 0; i < temp_h.size(); i++) 
		distance_h += std::abs(this->HITS_hub[i] - temp_h[i]);

	this->HITS_authority = temp_a;
	this->HITS_hub = temp_h;

	temp_a.clear();
	temp_h.clear();

	for (int i = 0; i < this->graph.nodes; i++) temp_a[i] = 1.;
	for (int i = 0; i < this->graph.nodes; i++) temp_h[i] = 1.;

	return distance_a > std::pow(10, -6) and distance_h > std::pow(10,-6);
}

// Function that normalizes the vectors in order to obtain a probability distribution.
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

// Function that gets the top-k nodes w.r.t. the authority score.
void HITS::get_topk_authority() {
	this->graph.get_algo_topk_results<int, double>(this->HITS_authority, this->top_k, this->authority_topk); 
}

// Function that gets the top-k nodes w.r.t. the hub score.
void HITS::get_topk_hub() {
	this->graph.get_algo_topk_results<int, double>(this->HITS_hub, this->top_k, this->hub_topk); 
}

// Function that prints the content of the authority vector.
void HITS::print_authority(){
	std::cout << "Values of a_k = [";
	for (int i = 0; i < this->HITS_authority.size(); i++){
		std::cout << HITS_authority[i] << ",";
	}
	std::cout << "]\n";
}

// Function that prints the content of the hub vector.
void HITS::print_hub(){
	std::cout << "Values of h_k = [";
	for (int i = 0; i < this->HITS_hub.size(); i++){
		std::cout << HITS_hub[i] << ",";
	}
	std::cout << "]\n";
}

// Function that prints the authority scores for the top-k nodes.
void HITS::print_topk_authority() {
	std::cout << "Authority scores" << std::endl;

	this->graph.print_algo_topk_results<double>(this->authority_topk, this->autority_str); 
}

// Function that prints the hub scores for the top-k nodes.
void HITS::print_topk_hub() {
	std::cout << "Hub scores" << std::endl;

	this->graph.print_algo_topk_results<double>(this->hub_topk, this->hub_str); 

}

// Function that prints the execution time and the number of steps taken by the HITS algorithm to converge.
void HITS::print_stats() {
	std::cout << "Elapsed: " << this->elapsed.count() << " ms \t Steps: "<< this->steps << std::endl;
}