/*******************************************************************************
module:   versioning_test
author:   digimokan
date:     28 SEP 2017 (created)
purpose:  test cases for functions in strace-4.6/versioning.c
*******************************************************************************/

#include "doctest.h"
#include "versioning.h"
#include "versioning_graph.h"

#include <stdio.h>

TEST_CASE("init_versioning") {

  VersionAction act;

  SUBCASE("normal init_versioning() call") {
    act = init_versioning();

    CHECK(act == SUCCESS_VERSIONING_INITIALIZED);
  }

  SUBCASE("repeat init_versioning() calls") {
    init_versioning();
    act = init_versioning();

    CHECK(act == ERR_VERSIONING_ALREADY_INITIALIZED);
  }

  clear_versioning();

}

TEST_CASE("clear_versioning") {

  VersionAction act;

  SUBCASE("normal clear_versioning() call") {
    init_versioning();
    act = clear_versioning();

    CHECK(act == SUCCESS_VERSIONING_CLEARED);
  }

  SUBCASE("clear_versioning() call prior to initialization") {
    act = clear_versioning();

    CHECK(act == ERR_VERSIONING_NOT_INITIALIZED);
  }

  clear_versioning();

}

TEST_CASE("get_versioning_graph") {

  struct versioned_prov_graph* vpg = NULL;

  SUBCASE("get uninitialized graph") {
    vpg = get_versioning_graph();

    CHECK_FALSE(vpg);
  }

  SUBCASE("get initialized graph") {
    init_versioning();
    vpg = get_versioning_graph();

    CHECK(vpg);
  }

  clear_versioning();

}

TEST_CASE("versioned_open") {

  VersionAction act;
  struct versioned_prov_graph* graph = NULL;

  char pid1[] = "1111";
  char pid1_keystr[] = "11111";
  struct node_entry* pid1_node = NULL;
  char file1[] = "file1";
  char file1_keystr[] = "file11";
  struct node_entry* file1_node = NULL;

  struct node_entry* found_node = NULL;

  struct edge_entry* edge1to2 = NULL;
  struct edge_entry* edge2to1 = NULL;

  init_versioning();

  SUBCASE("attempt versioned_open with uninitialized graph") {
    clear_versioning();
    act = versioned_open(pid1, file1, READ_ONLY);

    CHECK(act == ERR_VERSIONING_NOT_INITIALIZED);
  }

  SUBCASE("process opens file for write-only") {
    act = versioned_open(pid1, file1, WRITE_ONLY);

    CHECK(act == SUCCESS_FILE_VERSION_OPENED);

    graph = get_versioning_graph();

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    file1_node = retrieve_latest_versioned_node(graph, file1, FILE_NODE);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    CHECK(found_node == pid1_node);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, file1_keystr, strlen(file1_keystr), found_node);

    CHECK(found_node == file1_node);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, pid1_node, file1_node);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == ACTIVE);
  }

  SUBCASE("process opens file for read-write") {
    act = versioned_open(pid1, file1, READ_WRITE);

    CHECK(act == SUCCESS_FILE_VERSION_OPENED);

    graph = get_versioning_graph();

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    file1_node = retrieve_latest_versioned_node(graph, file1, FILE_NODE);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    CHECK(found_node == pid1_node);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, file1_keystr, strlen(file1_keystr), found_node);

    CHECK(found_node == file1_node);

    CHECK(HASH_CNT(hh, graph->edge_table) == 2);

    edge1to2 = find_edge_entry(graph, pid1_node, file1_node);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == ACTIVE);

    edge2to1 = find_edge_entry(graph, file1_node, pid1_node);

    REQUIRE(edge2to1);
    CHECK(edge2to1->edge_label == ACTIVE);
  }

  clear_versioning();

}

TEST_CASE("versioned_close") {

  VersionAction act;
  struct versioned_prov_graph* graph = NULL;

  char pid1[] = "1111";
  char pid1_keystr[] = "11111";
  struct node_entry* pid1_node = NULL;
  char file1[] = "file1";
  char file1_keystr[] = "file11";
  struct node_entry* file1_node = NULL;

  struct node_entry* found_node = NULL;

  struct edge_entry* edge1to2 = NULL;
  struct edge_entry* edge2to1 = NULL;

  init_versioning();

  SUBCASE("attempt versioned_close with uninitialized graph") {
    clear_versioning();
    act = versioned_close(pid1, file1, READ_ONLY);

    CHECK(act == ERR_VERSIONING_NOT_INITIALIZED);
  }

  SUBCASE("attempt close before a versioned_open") {
    graph = get_versioning_graph();

    act = versioned_close(pid1, file1, WRITE_ONLY);

    CHECK(act == SUCCESS_FILE_VERSION_CLOSED);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    file1_node = retrieve_latest_versioned_node(graph, file1, FILE_NODE);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    REQUIRE(found_node == pid1_node);
    CHECK(found_node->mark == UNMARKED);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, file1_keystr, strlen(file1_keystr), found_node);

    REQUIRE(found_node == file1_node);
    CHECK(found_node->mark == UNMARKED);

    CHECK(HASH_CNT(hh, graph->edge_table) == 0);
  }

  SUBCASE("versioned_close following versioned_open for write") {
    graph = get_versioning_graph();

    versioned_open(pid1, file1, WRITE_ONLY);
    act = versioned_close(pid1, file1, WRITE_ONLY);

    CHECK(act == SUCCESS_FILE_VERSION_CLOSED);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    file1_node = retrieve_latest_versioned_node(graph, file1, FILE_NODE);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    REQUIRE(found_node == pid1_node);
    CHECK(found_node->mark == MARKED);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, file1_keystr, strlen(file1_keystr), found_node);

    REQUIRE(found_node == file1_node);
    CHECK(found_node->mark == UNMARKED);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, pid1_node, file1_node);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);
  }

  SUBCASE("versioned_close following versioned_open for read-write") {
    graph = get_versioning_graph();

    versioned_open(pid1, file1, READ_WRITE);
    act = versioned_close(pid1, file1, READ_WRITE);

    CHECK(act == SUCCESS_FILE_VERSION_CLOSED);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    file1_node = retrieve_latest_versioned_node(graph, file1, FILE_NODE);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    REQUIRE(found_node == pid1_node);
    CHECK(found_node->mark == MARKED);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, file1_keystr, strlen(file1_keystr), found_node);

    REQUIRE(found_node == file1_node);
    CHECK(found_node->mark == MARKED);

    CHECK(HASH_CNT(hh, graph->edge_table) == 2);

    edge1to2 = find_edge_entry(graph, pid1_node, file1_node);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);

    edge2to1 = find_edge_entry(graph, file1_node, pid1_node);

    REQUIRE(edge2to1);
    CHECK(edge2to1->edge_label == INACTIVE);
  }

  clear_versioning();

}

TEST_CASE("versioned_spawn") {

  VersionAction act;
  struct versioned_prov_graph* graph = NULL;

  char pid1[] = "1111";
  char pid1_keystr[] = "11111";
  struct node_entry* pid1_node = NULL;
  char pid2[] = "2222";
  char pid2_keystr[] = "22221";
  struct node_entry* pid2_node = NULL;

  struct node_entry* found_node = NULL;

  struct edge_entry* edge1to2 = NULL;

  init_versioning();

  SUBCASE("attempt versioned_spawn with uninitialized graph") {
    clear_versioning();
    act = versioned_spawn(pid1, pid2);

    CHECK(act == ERR_VERSIONING_NOT_INITIALIZED);
  }

  SUBCASE("process 1 spawns process 2") {
    graph = get_versioning_graph();

    act = versioned_spawn(pid1, pid2);

    CHECK(HASH_CNT(hh, graph->node_table) == 2);

    pid1_node = retrieve_latest_versioned_node(graph, pid1, PROCESS_NODE);
    pid2_node = retrieve_latest_versioned_node(graph, pid2, PROCESS_NODE);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid1_keystr, strlen(pid1_keystr), found_node);

    CHECK(found_node == pid1_node);

    found_node = NULL;
    HASH_FIND(hh, graph->node_table, pid2_keystr, strlen(pid2_keystr), found_node);

    CHECK(found_node == pid2_node);

    CHECK(HASH_CNT(hh, graph->edge_table) == 1);

    edge1to2 = find_edge_entry(graph, pid1_node, pid2_node);

    REQUIRE(edge1to2);
    CHECK(edge1to2->edge_label == INACTIVE);
  }

  clear_versioning();

}

TEST_CASE("is_file_or_process_modified") {

  VersionAction act;
  struct versioned_prov_graph* graph = NULL;

  char process_p[]  = "P";
  char process_q[]  = "Q";

  char file_a[]  = "A";
  char file_b[]  = "B";

  init_versioning();

  versioned_open(process_p, file_a, WRITE_ONLY);
  versioned_close(process_p, file_a, WRITE_ONLY);

  SUBCASE("attempt is_file_modified with uninitialized graph") {
    clear_versioning();
    act = is_file_or_process_modified(file_a);

    CHECK(act == ERR_VERSIONING_NOT_INITIALIZED);
  }

  SUBCASE("is_modified for nonexistent file/process") {
    graph = get_versioning_graph();

    act = is_file_or_process_modified(file_b);

    CHECK(act == FILE_OR_PROCESS_NOT_EXIST);

    act = is_file_or_process_modified(process_q);

    CHECK(act == FILE_OR_PROCESS_NOT_EXIST);
  }

  SUBCASE("two nodes: unmodified file/process") {
    graph = get_versioning_graph();

    act = is_file_or_process_modified(file_a);

    CHECK(act == FILE_OR_PROCESS_NOT_MODIFIED);

    act = is_file_or_process_modified(process_p);

    CHECK(act == FILE_OR_PROCESS_NOT_MODIFIED);
  }

  SUBCASE("two nodes: modified file/process") {
    graph = get_versioning_graph();
    struct node_entry* file_a_node = get_node_entry_by_version(graph, file_a, 1);
    file_a_node->modflag = MODIFIED;
    struct node_entry* process_p_node = get_node_entry_by_version(graph, process_p, 1);
    process_p_node->modflag = MODIFIED;

    act = is_file_or_process_modified(file_a);

    CHECK(act == FILE_OR_PROCESS_MODIFIED);

    act = is_file_or_process_modified(process_p);

    CHECK(act == FILE_OR_PROCESS_MODIFIED);
  }

  clear_versioning();

}

  TEST_CASE("full-graph tests") {

  char process_p[] = "P";
  char process_q[] = "Q";
  char process_r[] = "R";
  char process_s[] = "S";

  char file_a[]  = "A";
  char file_b[]  = "B";
  char file_c[]  = "C";
  char file_d[]  = "D";
  char file_e[]  = "E";
  char file_f[]  = "F";

  struct edge_entry* found_edge = NULL;

  char edge_p1_to_p2_keystr[] = "P1P2";
  char edge_p1_to_q1_keystr[] = "P1Q1";
  char edge_p1_to_r1_keystr[] = "P1R1";
  char edge_p1_to_s1_keystr[] = "P1S1";
  char edge_p1_to_b1_keystr[] = "P1B1";
  char edge_p1_to_b2_keystr[] = "P1B2";
  char edge_q1_to_r1_keystr[] = "Q1R1";
  char edge_q1_to_a1_keystr[] = "Q1A1";
  char edge_q1_to_d1_keystr[] = "Q1D1";
  char edge_r1_to_d1_keystr[] = "R1D1";
  char edge_r1_to_f1_keystr[] = "R1F1";
  char edge_s1_to_b1_keystr[] = "S1B1";
  char edge_a1_to_p1_keystr[] = "A1P1";
  char edge_a1_to_p2_keystr[] = "A1P2";
  char edge_a1_to_r1_keystr[] = "A1R1";
  char edge_b1_to_b2_keystr[] = "B1B2";
  char edge_b1_to_q1_keystr[] = "B1Q1";
  char edge_b1_to_r1_keystr[] = "B1R1";
  char edge_b2_to_r1_keystr[] = "B2R1";
  char edge_c1_to_q1_keystr[] = "C1Q1";
  char edge_e1_to_r1_keystr[] = "E1R1";
  char edge_e1_to_s1_keystr[] = "E1S1";

  init_versioning();

  SUBCASE("end-to-end test 1a: write then read, same process") {

    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);
    versioned_open(process_p, file_a, READ_ONLY);
    versioned_close(process_p, file_a, READ_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b1_keystr, strlen(edge_p1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_p2_keystr, strlen(edge_p1_to_p2_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_a1_to_p2_keystr, strlen(edge_a1_to_p2_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 3);

  }

  SUBCASE("end-to-end test 1b: write then read, different processes") {

    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);
    versioned_open(process_q, file_b, READ_ONLY);
    versioned_close(process_q, file_b, READ_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b1_keystr, strlen(edge_p1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_q1_keystr, strlen(edge_b1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 2);

  }

  SUBCASE("end-to-end test 1c: read then write, different processes") {

    versioned_open(process_q, file_b, READ_ONLY);
    versioned_close(process_q, file_b, READ_ONLY);
    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_q1_keystr, strlen(edge_b1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_b2_keystr, strlen(edge_b1_to_b2_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b2_keystr, strlen(edge_p1_to_b2_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 3);

  }

  SUBCASE("end-to-end test 1d: duplicate vers graph paper experiment") {

    versioned_open(process_p, file_a, READ_ONLY);
    versioned_close(process_p, file_a, READ_ONLY);
    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);

    versioned_spawn(process_p, process_q);
    versioned_open(process_q, file_c, READ_ONLY);
    versioned_close(process_q, file_c, READ_ONLY);
    versioned_open(process_q, file_b, READ_ONLY);
    versioned_close(process_q, file_b, READ_ONLY);

    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);
    versioned_spawn(process_p, process_r);
    versioned_open(process_r, file_e, READ_ONLY);
    versioned_close(process_r, file_e, READ_ONLY);
    versioned_open(process_r, file_b, READ_ONLY);
    versioned_close(process_r, file_b, READ_ONLY);

    versioned_open(process_q, file_d, WRITE_ONLY);
    versioned_close(process_q, file_d, WRITE_ONLY);
    versioned_open(process_r, file_f, WRITE_ONLY);
    versioned_close(process_r, file_f, WRITE_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_a1_to_p1_keystr, strlen(edge_a1_to_p1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b1_keystr, strlen(edge_p1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_q1_keystr, strlen(edge_p1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_c1_to_q1_keystr, strlen(edge_c1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_q1_keystr, strlen(edge_b1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_b2_keystr, strlen(edge_b1_to_b2_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b2_keystr, strlen(edge_p1_to_b2_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_r1_keystr, strlen(edge_p1_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_e1_to_r1_keystr, strlen(edge_e1_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b2_to_r1_keystr, strlen(edge_b2_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_q1_to_d1_keystr, strlen(edge_q1_to_d1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_r1_to_f1_keystr, strlen(edge_r1_to_f1_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 12);

  }

  SUBCASE("end-to-end test 2a: interlaced multiprocess read/write withoutj closes") {

    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_open(process_q, file_b, READ_ONLY);
    versioned_open(process_q, file_a, WRITE_ONLY);
    versioned_open(process_p, file_a, READ_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b1_keystr, strlen(edge_p1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_q1_keystr, strlen(edge_b1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_q1_to_a1_keystr, strlen(edge_q1_to_a1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_a1_to_p1_keystr, strlen(edge_a1_to_p1_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 4);

  }

  SUBCASE("end-to-end test 3a: sequential write-read-write chain") {

    versioned_open(process_p, file_b, WRITE_ONLY);
    versioned_close(process_p, file_b, WRITE_ONLY);
    versioned_open(process_q, file_b, READ_ONLY);
    versioned_close(process_q, file_b, READ_ONLY);
    versioned_open(process_q, file_a, WRITE_ONLY);
    versioned_close(process_q, file_a, WRITE_ONLY);
    versioned_open(process_p, file_a, READ_ONLY);
    versioned_close(process_p, file_a, READ_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_b1_keystr, strlen(edge_p1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_q1_keystr, strlen(edge_b1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_q1_to_a1_keystr, strlen(edge_q1_to_a1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_p2_keystr, strlen(edge_p1_to_p2_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_a1_to_p2_keystr, strlen(edge_a1_to_p2_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 5);

  }

  SUBCASE("end-to-end test 4a: interlaced and chained read/write") {

    versioned_spawn(process_p, process_q);
    versioned_open(process_q, file_c, READ_ONLY);
    versioned_spawn(process_q, process_r);
    versioned_open(process_q, file_a, WRITE_ONLY);
    versioned_close(process_q, file_c, READ_ONLY);
    versioned_close(process_q, file_a, WRITE_ONLY);
    versioned_open(process_r, file_a, READ_ONLY);
    versioned_close(process_r, file_a, READ_ONLY);
    versioned_spawn(process_p, process_s);
    versioned_open(process_s, file_e, READ_ONLY);
    versioned_close(process_s, file_e, READ_ONLY);
    versioned_open(process_s, file_b, WRITE_ONLY);
    versioned_open(process_r, file_b, READ_ONLY);
    versioned_open(process_r, file_d, WRITE_ONLY);
    versioned_close(process_r, file_d, WRITE_ONLY);
    versioned_close(process_r, file_b, READ_ONLY);
    versioned_close(process_s, file_b, WRITE_ONLY);

    struct versioned_prov_graph* graph = get_versioning_graph();

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_q1_keystr, strlen(edge_p1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_c1_to_q1_keystr, strlen(edge_c1_to_q1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_q1_to_r1_keystr, strlen(edge_q1_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_q1_to_a1_keystr, strlen(edge_q1_to_a1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_a1_to_r1_keystr, strlen(edge_a1_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_p1_to_s1_keystr, strlen(edge_p1_to_s1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_e1_to_s1_keystr, strlen(edge_e1_to_s1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_s1_to_b1_keystr, strlen(edge_s1_to_b1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_b1_to_r1_keystr, strlen(edge_b1_to_r1_keystr), found_edge);

    CHECK(found_edge);

    found_edge = NULL;
    HASH_FIND(hh, graph->edge_table, edge_r1_to_d1_keystr, strlen(edge_r1_to_d1_keystr), found_edge);

    CHECK(found_edge);

    CHECK(HASH_CNT(hh, graph->edge_table) == 10);

  }

  clear_versioning();

}

