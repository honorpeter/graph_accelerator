#include "graph.h"

void Graph::loadFile(
        const std::string& fname, 
        std::vector<std::vector<int>> &data
        ){

    std::ifstream fhandle(fname.c_str());
    if(!fhandle.is_open()){
        HERE;
        std::cout << "Failed to open " << fname << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while(std::getline(fhandle, line)){
        std::istringstream iss(line);
        data.push_back(
                std::vector<int>(std::istream_iterator<int>(iss),
                    std::istream_iterator<int>())
                );
    }
    fhandle.close();
}

// Check the number of vertices without out going neighbors,
// as it affects the BFS results.
void Graph::getStat(){
    int zero_outgoing_vertex_num = 0;
    for(auto it = vertices.begin(); it != vertices.end(); it++){
        if((*it)->out_vids.empty()){
            zero_outgoing_vertex_num++;
        }
    }
    std::cout << "Zero outgoing vertex percentage is " << zero_outgoing_vertex_num * 1.0 / vertex_num << std::endl;
}


int Graph::getMaxIdx(const std::vector<std::vector<int>> &data){
    int max_idx = data[0][0]; 
    for(auto it1 = data.begin(); it1 != data.end(); it1++){
        for(auto it2 = it1->begin(); it2 != it1->end(); it2++){            
            if(max_idx <= (*it2)){
                max_idx = *it2;
            }
        }
    }
    return max_idx;
}

int Graph::getMinIdx(const std::vector<std::vector<int>> &data){
    int min_idx = data[0][0]; 
    for(auto it1 = data.begin(); it1 != data.end(); it1++){
        for(auto it2 = it1->begin(); it2 != it1->end(); it2++){            
            if(min_idx >= (*it2)){
                min_idx = *it2;
            }
        }
    }
    return min_idx;
}

void Graph::getRandomStartIndices(std::vector<int> &start_indices){
    size_t num = start_indices.size();
    start_indices.clear();
    size_t n = 0;
    while(n < num){
        int max_idx = vertex_num - 1;
        int idx = rand()%max_idx;
        if(vertices[idx]->out_vids.empty() || std::find(start_indices.begin(), start_indices.end(), idx) != start_indices.end()){
            continue;
        }
        start_indices.push_back(idx);
        n++;
    }
}


Graph::Graph(const std::string& fname){

    std::vector<std::vector<int>> data;
    loadFile(fname, data);

    if(isValidData(data) == false){
        HERE;
        std::cout << "The data is invalid. " << std::endl;
        exit(EXIT_FAILURE);
    }

    vertex_num = getMaxIdx(data) + 1;
    edge_num = (int)data.size();
    std::cout << "vertex num: " << vertex_num << std::endl;
    std::cout << "edge num: " << edge_num << std::endl;

    for(int i = 0; i < vertex_num; i++){
        Vertex* v = new Vertex(i);
        vertices.push_back(v);
    }

    for(auto it = data.begin(); it != data.end(); it++){
        int src_idx = (*it)[0];
        int dst_idx = (*it)[1];
        vertices[src_idx]->out_vids.push_back(dst_idx);
        vertices[dst_idx]->in_vids.push_back(src_idx);
    }

    for(auto it = vertices.begin(); it != vertices.end(); it++){
        (*it)->in_deg = (int)(*it)->in_vids.size();
        (*it)->out_deg = (int)(*it)->out_vids.size();
    }
}

CSR::CSR(const Graph &g) : v_num(g.vertex_num), e_num(g.edge_num){
    // Assign random data to weight though it is not used in bfs.
    weight.resize(e_num);
    for(auto &w : weight){
        w = (rand()%100)/10.0;
    }

    rpao.resize(v_num+1);
    rpai.resize(v_num+1);
    rpao[0] = 0;
    rpai[0] = 0;
    for(int i = 0; i < v_num; i++){
        rpao[i+1] = rpao[i] + g.vertices[i]->out_deg;
        rpai[i+1] = rpai[i] + g.vertices[i]->in_deg;
    }

    for(int i = 0; i < v_num; i++){
        for(auto id : g.vertices[i]->out_vids){
            ciao.push_back(id);
        }
        for(auto id : g.vertices[i]->in_vids){
            ciai.push_back(id);
        }
    }
}

// This function will not change the data in CSR
bool CSR::basic_bfs(const int &start_idx){
    int level = 0;
    std::vector<int> depth;
    depth.resize(v_num);
    std::fill(depth.begin(), depth.end(), -1);

    depth[start_idx] = true;
    std::vector<int> frontier;
    frontier.push_back(start_idx);

    while(!frontier.empty()){
        std::vector<int> next_frontier;

        // Traverse the frontier
        for(auto vidx : frontier){
            for(int cidx = rpao[vidx]; cidx < rpao[vidx+1]; cidx++){
                int out_ngb = ciao[cidx];
                if(depth[out_ngb] == -1){
                    depth[out_ngb] = level + 1;
                    next_frontier.push_back(out_ngb);
                }
            }
        }

        // Update frontier
        frontier = next_frontier;
        level++;
    }

    std::string fname = "gold.txt";
    std::ofstream fhandle(fname.c_str());
    if(!fhandle.is_open()){
        HERE;
        std::cout << "Failed to open " << fname << std::endl;
        exit(EXIT_FAILURE);
    }
    for(auto d : depth){
        std::cout << d << " ";
    }
    std::cout << std::endl;

    return true;
}

bool CSR::bfs(const int &start_idx){
    int level = 0;
    std::vector<int> depth;
    depth.resize(v_num);
    std::fill(depth.begin(), depth.end(), -1);
    depth[start_idx] = 0;

    //std::vector<int, int> hub_vertex_depth;
    std::vector<int> lrg_frontier;
    std::vector<int> mid_frontier;
    std::vector<int> sml_frontier;

    int hub_vertex_threshold = 4096;
    int lrg_threshold = 1024;
    int sml_threshold = 16;
    int total_hub_vertex_num = getHubVertexNum(hub_vertex_threshold);
    bool visited_ngb_num = 0;
    bool top_down = true;
    bool end_of_bfs;
    do{
        // Clean the frontier container
        lrg_frontier.clear();
        mid_frontier.clear();
        sml_frontier.clear();

        // get frontier and classfiy the frontier
        int frontier_hub_vertex_num = 0;
        if(top_down){
            for(int idx = 0; idx < v_num; idx++){
                if(depth[idx] == level){ 
                    int degree = rpao[idx+1] - rpao[idx];
                    if(degree <= sml_threshold){
                        sml_frontier.push_back(idx);
                    }
                    else if(degree >= lrg_threshold){
                        lrg_frontier.push_back(idx);
                    }
                    else{
                        mid_frontier.push_back(idx);
                    }

                    // hub vertex buffer may be updated here.
                    if(degree >= hub_vertex_threshold){
                        hub_vertex_threshold++;
                    }
                }
            }
        }
        else{
            for(int idx = 0; idx < v_num; idx++){
                if(depth[idx] == -1){
                    int degree = rpai[idx+1] - rpai[idx];
                    if(degree <= sml_threshold){
                        sml_frontier.push_back(idx);
                    }
                    else if(degree >= lrg_threshold){
                        lrg_frontier.push_back(idx);
                    }
                    else{
                        mid_frontier.push_back(idx);
                    }

                    if(degree >= hub_vertex_threshold){
                        hub_vertex_threshold++;
                    }
                }
            }
        }

        // Traverse the frontier
        if(top_down){
            // top-down traverse
            auto traverse = [&depth, level, this](const std::vector<int> &frontier){
                for(auto vidx : frontier){
                    for(int cidx = rpao[vidx]; cidx < rpao[vidx+1]; cidx++){
                        int out_ngb = ciao[cidx];
                        if(depth[out_ngb] == -1){
                            depth[out_ngb] = level + 1; 
                        }
                    }
                }
            };

            traverse(lrg_frontier);
            traverse(mid_frontier);
            traverse(sml_frontier);
        }
        else{
            // bottom-up traverse
            // When none of the frontier has a visited incoming neighboring, 
            // it also indicates the end of the BFS. 
            visited_ngb_num = 0;
            auto traverse = [&depth, &visited_ngb_num, level, this](const std::vector<int> &frontier){
                for(auto vidx : frontier){
                    for(int cidx = rpai[vidx]; cidx < rpai[vidx+1]; cidx++){
                        int in_ngb = ciai[cidx];
                        if(depth[in_ngb] == level){
                            depth[vidx] = level + 1;
                            visited_ngb_num++;
                            break;
                        }
                    }
                }
            };

            traverse(lrg_frontier);
            traverse(mid_frontier);
            traverse(sml_frontier);

        }

        float hub_percentage = frontier_hub_vertex_num * 1.0 / total_hub_vertex_num; 
        if(hub_percentage >= 0.3){
            top_down = false;
        }

        // update depth
        level++;
        end_of_bfs = lrg_frontier.empty() && mid_frontier.empty() && sml_frontier.empty();
        end_of_bfs = end_of_bfs || (top_down == false && visited_ngb_num == 0);

    } while(!end_of_bfs); 

    /*
    for(auto d : depth){
        std::cout << d << " ";
    }
    std::cout << std::endl;
    */

}

int CSR::getHubVertexNum(const int &threshold){
    int hub_vertex_num = 0;
    for(int idx = 0; idx < v_num; idx++){
        int degree = rpao[idx+1] - rpao[idx];
        if(degree >= threshold){
            hub_vertex_num++;
        }
    }

    // Fix the situation when there is no hub verties
    if(hub_vertex_num == 0)
        hub_vertex_num = 1;

    return hub_vertex_num;
}

