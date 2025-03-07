#include <sys/stat.h>

#include <iostream>
#include <string>

#include <gflags/gflags.h>

#include "graphs/edge_list.h"
#include "graphs/immutable_csr.h"
#include "portability/sys_data_structure.h"
#include "portability/sys_types.h"
#include "utility/io/data_mngr.h"
#include "utility/io/edge_list_io_adapter.h"
#include "utility/paritioner/edge_cut_partitioner.h"
#include "utility/paritioner/partitioner_base.h"

using CSR_T = minigraph::graphs::ImmutableCSR<gid_t, vid_t, vdata_t, edata_t>;
using GRAPH_BASE_T = minigraph::graphs::Graph<gid_t, vid_t, vdata_t, edata_t>;
using EDGE_LIST_T = minigraph::graphs::EdgeList<gid_t, vid_t, vdata_t, edata_t>;

void EdgeList2CSR(std::string src_pt, std::string dst_pt, std::size_t cores,
                  char separator_params = ',', const bool frombin = false) {
  minigraph::utility::io::DataMngr<CSR_T> data_mngr;

  minigraph::utility::partitioner::PartitionerBase<CSR_T>* partitioner =
      nullptr;
  partitioner =
      new minigraph::utility::partitioner::EdgeCutPartitioner<CSR_T>();
  minigraph::utility::io::EdgeListIOAdapter<gid_t, vid_t, vdata_t, edata_t>
      edgelist_io_adapter;

  auto edgelist_graph = new EDGE_LIST_T;
  if (frombin) {
    std::string meta_pt = src_pt + "minigraph_meta" + ".bin";
    std::string data_pt = src_pt + "minigraph_data" + ".bin";
    std::string vdata_pt = src_pt + "minigraph_vdata" + ".bin";

    edgelist_io_adapter.ReadEdgeListFromBin((GRAPH_BASE_T*)edgelist_graph, 0,
                                            meta_pt, data_pt, vdata_pt);
  } else {
    edgelist_io_adapter.ParallelReadEdgeListFromCSV(
        (GRAPH_BASE_T*)edgelist_graph, src_pt, 0, separator_params, cores);
  }

  partitioner->ParallelPartition(edgelist_graph, 1, cores);

  if (!data_mngr.Exist(dst_pt + "minigraph_meta/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_meta/");
  } else {
    remove((dst_pt + "minigraph_meta/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_meta/");
  }
  if (!data_mngr.Exist(dst_pt + "minigraph_data/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_data/");
  } else {
    remove((dst_pt + "minigraph_data/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_data/");
  }
  if (!data_mngr.Exist(dst_pt + "minigraph_vdata/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_vdata/");
  } else {
    remove((dst_pt + "minigraph_vdata/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_vdata/");
  }
  if (!data_mngr.Exist(dst_pt + "minigraph_border_vertexes/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_border_vertexes/");
  } else {
    remove((dst_pt + "minigraph_border_vertexes/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_border_vertexes/");
  }
  if (!data_mngr.Exist(dst_pt + "minigraph_message/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_message/");
  } else {
    remove((dst_pt + "minigraph_message/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_message/");
  }

  if (!data_mngr.Exist(dst_pt + "minigraph_message/")) {
    data_mngr.MakeDirectory(dst_pt + "minigraph_message/");
  } else {
    remove((dst_pt + "minigraph_message/").c_str());
    data_mngr.MakeDirectory(dst_pt + "minigraph_message/");
  }

  auto fragments = partitioner->GetFragments();
  size_t count = 0;
  for (auto& iter_fragments : *fragments) {
    auto fragment = (CSR_T*)iter_fragments;
    std::string meta_pt =
        dst_pt + "minigraph_meta/" + std::to_string(count) + ".bin";
    std::string data_pt =
        dst_pt + "minigraph_data/" + std::to_string(count) + ".bin";
    std::string vdata_pt =
        dst_pt + "minigraph_vdata/" + std::to_string(count) + ".bin";
    data_mngr.csr_io_adapter_->Write(*fragment, csr_bin, false, meta_pt,
                                     data_pt, vdata_pt);
    count++;
  }

  LOG_INFO("WriteCommunicationMatrix.");
  auto pair_communication_matrix = partitioner->GetCommunicationMatrix();

  LOG_INFO("WriteVidMap.");
  auto vid_map = partitioner->GetVidMap();
  if (vid_map != nullptr)
    data_mngr.WriteVidMap(partitioner->GetMaxVid(), vid_map,
                          dst_pt + "minigraph_message/vid_map.bin");

  auto global_border_vid_map = partitioner->GetGlobalBorderVidMap();
  if (global_border_vid_map != nullptr) {
    LOG_INFO("WriteGlobalBorderVidMap. size: ", global_border_vid_map->size_);
    data_mngr.WriteBitmap(
        global_border_vid_map,
        dst_pt + "minigraph_message/global_border_vid_map.bin");
  }

  remove(
      (dst_pt + "minigraph_border_vertexes/communication_matrix.bin").c_str());
  remove((dst_pt + "minigraph_message/vid_map.bin").c_str());
  remove((dst_pt + "minigraph_message/global_border_vid_map.bin").c_str());
  data_mngr.WriteCommunicationMatrix(
      dst_pt + "minigraph_border_vertexes/communication_matrix.bin",
      pair_communication_matrix.second, pair_communication_matrix.first);
  LOG_INFO("End graph convert#");
}

void EdgeListCSV2EdgeListBin(std::string src_pt, std::string dst_pt,
                             const size_t cores, char separator_params = ',') {
  std::cout << " #Converting " << FLAGS_t << ": input: " << src_pt
            << " output: " << dst_pt << std::endl;
  minigraph::utility::io::EdgeListIOAdapter<gid_t, vid_t, vdata_t, edata_t>
      edge_list_io_adapter;
  auto graph = new EDGE_LIST_T;
  std::string meta_pt = dst_pt + "minigraph_meta" + ".bin";
  std::string data_pt = dst_pt + "minigraph_data" + ".bin";
  std::string vdata_pt = dst_pt + "minigraph_vdata" + ".bin";
  edge_list_io_adapter.ParallelReadEdgeListFromCSV((GRAPH_BASE_T*)graph, src_pt,
                                                   0, separator_params, cores);
  LOG_INFO("Write: ", meta_pt);
  LOG_INFO("Write: ", data_pt);
  LOG_INFO("Write: ", vdata_pt);
  edge_list_io_adapter.Write(*graph, edge_list_bin, meta_pt, data_pt, vdata_pt);
}

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  assert(FLAGS_i != "" && FLAGS_o != "");
  std::string src_pt = FLAGS_i;
  std::string dst_pt = FLAGS_o;
  std::size_t cores = FLAGS_cores;
  std::string graph_type = FLAGS_t;

  if (FLAGS_tobin) {
    if (graph_type == "edgelist_bin") {
      EdgeListCSV2EdgeListBin(src_pt, dst_pt, cores, *FLAGS_sep.c_str());
    } else if (graph_type == "csr_bin") {
      EdgeList2CSR(src_pt, dst_pt, cores, *FLAGS_sep.c_str(), FLAGS_frombin);
    }
  }
  gflags::ShutDownCommandLineFlags();
}