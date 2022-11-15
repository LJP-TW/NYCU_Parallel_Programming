// Ref: https://parlab.eecs.berkeley.edu/sites/all/parlab/files/main.pdf
#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <cstdint>
#include <omp.h>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set *list)
{
    list->count = 0;
}

void vertex_set_init(vertex_set *list, int count)
{
    list->max_vertices = count;
    list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
    vertex_set_clear(list);
}

void vertex_set_release(vertex_set *list)
{
    free(list->vertices);
}

/*
 * bitmap[0]:  63 ~  0
 * bitmap[1]: 127 ~ 64
 * ...
 */
struct bitmap_t {
  int count; // # of 1 bits in bitmap
  int size; // bitmap size
  uint64_t *bitmap;
};

inline void bitmap_clear(bitmap_t *bitmap);

void bitmap_init(bitmap_t *bitmap, int bitcount)
{
    bitmap->size = (bitcount + 63) / 64;
    bitmap->bitmap = (uint64_t *)malloc(sizeof(uint64_t) * bitmap->size);

    bitmap_clear(bitmap);
}

void bitmap_release(bitmap_t *bitmap)
{
    free(bitmap->bitmap);
}

inline void bitmap_clear(bitmap_t *bitmap)
{
    #pragma omp parallel for
    for (int i = 0; i < bitmap->size; ++i)
    {
        bitmap->bitmap[i] = 0;
    }
    bitmap->count = 0;
}

inline uint64_t bitmap_get(bitmap_t *bitmap, int bit)
{
    // (bitmap->bitmap[bit / 64] >> (bit & 63)) & 1
    return (bitmap->bitmap[bit >> 6] >> (bit & 0x3f)) & 1;
}

inline void bitmap_set(bitmap_t *bitmap, int bit)
{
    __sync_fetch_and_add(&bitmap->count, 1);
    __sync_fetch_and_or(&bitmap->bitmap[bit >> 6], (uint64_t)1 << (bit & 0x3f));
}

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// next frontier.
void top_down_step(
    Graph g,
    vertex_set *frontier,
    vertex_set *next,
    int *distances)
{
    int num_edges = g->num_edges;
    int num_nodes = g->num_nodes;
    int *outgoing_starts = g->outgoing_starts;
    Vertex *outgoing_edges = g->outgoing_edges;

    #pragma omp parallel for
    for (int i = 0; i < frontier->count; i++)
    {
        Vertex v = frontier->vertices[i];

        int start_edge = outgoing_starts[v];
        int end_edge = (v == num_nodes - 1)
                           ? num_edges
                           : outgoing_starts[v + 1];

        // attempt to add all neighbors to the new frontier
        for (int neighbor = start_edge; neighbor < end_edge; neighbor++)
        {
            Vertex child_v = outgoing_edges[neighbor];

            if (distances[child_v] == NOT_VISITED_MARKER)
            {
                distances[child_v] = distances[v] + 1;
                int index = __sync_fetch_and_add(&next->count, 1);
                next->vertices[index] = child_v;
            }
        }
    }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol)
{

    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    vertex_set *frontier = &list1;
    vertex_set *next = &list2;

    // initialize all nodes to NOT_VISITED
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    frontier->vertices[frontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {

#ifdef VERBOSE
        double start_time = CycleTimer::currentSeconds();
#endif

        vertex_set_clear(next);

        top_down_step(graph, frontier, next, sol->distances);

#ifdef VERBOSE
        double end_time = CycleTimer::currentSeconds();
        printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

        // swap pointers
        vertex_set *tmp = frontier;
        frontier = next;
        next = tmp;
    }

    vertex_set_release(&list1);
    vertex_set_release(&list2);    
}

void bottom_up_step(
    Graph g,
    bitmap_t *frontier,
    bitmap_t *next,
    int *distances)
{
    int num_edges = g->num_edges;
    int num_nodes = g->num_nodes;
    int *incoming_starts = g->incoming_starts;
    Vertex *incoming_edges = g->incoming_edges;

    #pragma omp parallel for schedule(monotonic: dynamic, 1024)
    for (Vertex v = 0; v < num_nodes; ++v)
    {
        if (distances[v] != NOT_VISITED_MARKER)
            continue;

        int start_edge = incoming_starts[v];
        int end_edge = (v == num_nodes - 1)
                       ? num_edges
                       : incoming_starts[v + 1];

        for (int edgeidx = start_edge; edgeidx < end_edge; ++edgeidx)
        {
            Vertex parent_v = incoming_edges[edgeidx];

            if (bitmap_get(frontier, parent_v))
            {
                distances[v] = distances[parent_v] + 1;
                bitmap_set(next, v);
                break;
            }
        }
    }
}

void bfs_bottom_up(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "bottom up" BFS here as
    // described in the handout.
    //
    // As a result of your code's execution, sol.distances should be
    // correctly populated for all nodes in the graph.
    //
    // As was done in the top-down case, you may wish to organize your
    // code by creating subroutine bottom_up_step() that is called in
    // each step of the BFS process.
    // initialize all nodes to NOT_VISITED
    bitmap_t bitmap1;
    bitmap_t bitmap2;
    bitmap_init(&bitmap1, graph->num_nodes);
    bitmap_init(&bitmap2, graph->num_nodes);

    bitmap_t *frontier = &bitmap1;
    bitmap_t *next = &bitmap2;

    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    bitmap_set(frontier, ROOT_NODE_ID);
    sol->distances[ROOT_NODE_ID] = 0;

    while (frontier->count != 0)
    {
        bitmap_clear(next);

        bottom_up_step(graph, frontier, next, sol->distances);

        // swap pointers
        bitmap_t *tmp = frontier;
        frontier = next;
        next = tmp;
    }

    bitmap_release(&bitmap1);
    bitmap_release(&bitmap2);
}

void bfs_hybrid(Graph graph, solution *sol)
{
    // For PP students:
    //
    // You will need to implement the "hybrid" BFS here as
    // described in the handout.

    // Please see reference paper
    int mf;      // # of edges to check from the frontier
    int nf;      // # of vertices in the frontier
    int mu;      // # of edges to check from unexplored vertices
    int a = 14;  // tuning parameter
    int b = 24;  // tuning parameter
    int run_top_down = 1;
    int needConvert = 0;

    vertex_set list1;
    vertex_set list2;
    vertex_set_init(&list1, graph->num_nodes);
    vertex_set_init(&list2, graph->num_nodes);

    bitmap_t bitmap1;
    bitmap_t bitmap2;
    bitmap_init(&bitmap1, graph->num_nodes);
    bitmap_init(&bitmap2, graph->num_nodes);

    vertex_set *vfrontier = &list1;
    vertex_set *vnext = &list2;

    bitmap_t *bfrontier = &bitmap1;
    bitmap_t *bnext = &bitmap2;

    // initialize all nodes to NOT_VISITED
    for (int i = 0; i < graph->num_nodes; i++)
        sol->distances[i] = NOT_VISITED_MARKER;

    // setup frontier with the root node
    vfrontier->vertices[vfrontier->count++] = ROOT_NODE_ID;
    sol->distances[ROOT_NODE_ID] = 0;

    while (vfrontier->count != 0)
    {
        if ((run_top_down && vfrontier->count == 0)
            || (!run_top_down && bfrontier->count == 0))
            break;

        if (needConvert)
        {
            printf("Convert!\n");
            if (run_top_down)
            {
                // vfrontier -> bfrontier
                bitmap_clear(bfrontier);
                for (int i = 0; i < vfrontier->count; ++i)
                {
                    bitmap_set(bfrontier, vfrontier->vertices[i]);
                }
            }
            else
            {
                // bfrontier -> vfrontier
                vertex_set_clear(vfrontier);
                for (int i = 0; i < bfrontier->size; ++i)
                {
                    uint64_t map = bfrontier->bitmap[i];
                    int idx = 0;

                    while (map)
                    {
                        if (map & 1)
                        {
                            vfrontier->vertices[vfrontier->count++] = i * 64 + idx;
                        }
                        map >>= 1;
                        ++idx;
                    }
                }
            }

            run_top_down = !run_top_down;
            needConvert = 0;
        }

        if (run_top_down)
        {
            vertex_set_clear(vnext);

            top_down_step(graph, vfrontier, vnext, sol->distances);

            mf = 0;
            mu = 0;

            for (int i = 0; i < vnext->count; i++)
            {
                Vertex v = vnext->vertices[i];

                int start_edge = graph->outgoing_starts[v];
                int end_edge = (v == graph->num_nodes - 1)
                                   ? graph->num_edges
                                   : graph->outgoing_starts[v + 1];

                mf += end_edge - start_edge;
            }

            for (Vertex v = 0; v < graph->num_nodes; ++v)
            {
                if (sol->distances[v] != NOT_VISITED_MARKER)
                    continue;

                int start_edge = graph->incoming_starts[v];
                int end_edge = (v == graph->num_nodes - 1)
                               ? graph->num_edges
                               : graph->incoming_starts[v + 1];

                mu += end_edge - start_edge;
            }

            // swap pointers
            vertex_set *tmp = vfrontier;
            vfrontier = vnext;
            vnext = tmp;

            if (mf * a > mu)
            {
                needConvert = 1;
            }
        }
        else
        {
            bitmap_clear(bnext);

            bottom_up_step(graph, bfrontier, bnext, sol->distances);

            nf = bnext->count;

            // swap pointers
            bitmap_t *tmp = bfrontier;
            bfrontier = bnext;
            bnext = tmp;

            if (b * nf < graph->num_nodes)
            {
                needConvert = 1;
            }
        }
    }

    vertex_set_release(&list1);
    vertex_set_release(&list2);
    bitmap_release(&bitmap1);
    bitmap_release(&bitmap2);
}
