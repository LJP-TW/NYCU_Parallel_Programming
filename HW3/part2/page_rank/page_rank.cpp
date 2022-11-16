#include "page_rank.h"

#include <stdlib.h>
#include <cmath>
#include <omp.h>
#include <utility>

#include <vector>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

// pageRank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is num_nodes(g))
// damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void pageRank(Graph g, double *solution, double damping, double convergence)
{

  // initialize vertex weights to uniform probability. Double
  // precision scores are used to avoid underflow for large graphs

  /*
    For PP students: Implement the page rank algorithm here.  You
    are expected to parallelize the algorithm using openMP.  Your
    solution may need to allocate (and free) temporary arrays.

    Basic page rank pseudocode is provided below to get you started:

    // initialization: see example code above
    score_old[vi] = 1/numNodes;

    while (!converged) {

      // compute score_new[vi] for all nodes vi:
      score_new[vi] = sum over all nodes vj reachable from incoming edges
                         { score_old[vj] / number of edges leaving vj  }
      score_new[vi] = (damping * score_new[vi]) + (1.0-damping) / numNodes;

      score_new[vi] += sum over all nodes v in graph with no outgoing edges
                         { damping * score_old[v] / numNodes }

      // compute how much per-node scores have changed
      // quit once algorithm has converged

      global_diff = sum over all nodes vi { abs(score_new[vi] - score_old[vi]) };
      converged = (global_diff < convergence)
    }

  */
  int num_nodes = g->num_nodes;
  int num_edges = g->num_edges;
  int *outgoing_starts = g->outgoing_starts;
  Vertex *outgoing_edges = g->outgoing_edges;
  int *incoming_starts = g->incoming_starts;
  Vertex *incoming_edges = g->incoming_edges;

  double global_diff;
  double *solution_old = new double[num_nodes];
  double equal_prob = 1.0 / num_nodes;
  std::vector<Vertex> partial_vs[omp_get_max_threads()];
  std::vector<Vertex> other_vs; // all nodes v in graph with no outgoing edges
  double partial_scores[omp_get_max_threads()];
  double no_out_score;

  #pragma omp parallel for
  for (Vertex v = 0; v < num_nodes; ++v)
  {
    int start_edge = outgoing_starts[v];
    int end_edge = (v == num_nodes - 1)
                   ? num_edges
                   : outgoing_starts[v + 1];
    
    if (start_edge == end_edge)
    {
      partial_vs[omp_get_thread_num()].push_back(v);
    }
  }

  for (int i = 0; i < omp_get_max_threads(); ++i)
  {
    for (Vertex v : partial_vs[i])
    {
      other_vs.push_back(v);
    }
  }

  #pragma omp parallel for
  for (Vertex i = 0; i < num_nodes; ++i)
  {
    solution_old[i] = equal_prob;
  }

  while (1)
  {
    // sum over all nodes incoming_v reachable from incoming edges
    // score[v] = sum(score_old[incoming_v] / 
    //   number of edges leaving incoming_v) for each incoming_v

    #pragma omp parallel for
    for (Vertex v = 0; v < num_nodes; ++v)
    {
      solution[v] = 0;
      int start_edge = incoming_starts[v];
      int end_edge = (v == num_nodes - 1)
                     ? num_edges
                     : incoming_starts[v + 1];

      for (int edgeidx = start_edge; edgeidx < end_edge; ++edgeidx)
      {
        Vertex incoming_v = incoming_edges[edgeidx];

        int out_start_edge = outgoing_starts[incoming_v];
        int out_end_edge = (incoming_v == num_nodes - 1)
                           ? num_edges
                           : outgoing_starts[incoming_v + 1];
        solution[v] += (solution_old[incoming_v] / (out_end_edge - out_start_edge));
      }
    }

    // damping & sum over all nodes other_v in graph with no outgoing edges
    // score[v] = (damping * score[v]) + (1.0-damping) / num_nodes;
    // score[v] += sum(damping * score_old[other_v] /
    //   num_nodes) for each other_v
    // --->
    // score[v] = (damping * score[v]) + 
    //   ((damping * sum(score_old[other_v]) for each other_v) + (1.0-damping)) / num_nodes

    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
      partial_scores[i] = 0;
    }

    #pragma omp parallel for
    for (int i = 0; i < other_vs.size(); ++i)
    {
      Vertex other_v = other_vs[i];
      partial_scores[omp_get_thread_num()] += solution_old[other_v];
    }

    no_out_score = 0;
    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
      no_out_score += partial_scores[i];
    }

    no_out_score = ((damping * no_out_score) + (1.0 - damping)) / num_nodes;

    #pragma omp parallel for
    for (Vertex v = 0; v < num_nodes; ++v)
    {
      solution[v] = damping * solution[v] + no_out_score;
    }

    // compute how much per-node scores have changed
    // quit once algorithm has converged

    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
      partial_scores[i] = 0;
    }

    #pragma omp parallel for
    for (Vertex v = 0; v < num_nodes; ++v)
    {
      double diff = solution[v] - solution_old[v];
      partial_scores[omp_get_thread_num()] += diff >= 0 ? diff : -diff;
    }

    global_diff = 0;
    for (int i = 0; i < omp_get_max_threads(); ++i)
    {
      global_diff += partial_scores[i];
    }

    if (global_diff < convergence)
      break;

    #pragma omp parallel for
    for (int i = 0; i < num_nodes; ++i)
    {
      solution_old[i] = solution[i];
    }
  }

  delete [] solution_old;
}
