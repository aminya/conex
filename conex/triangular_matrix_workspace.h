#pragma once
#include <numeric>
#include <vector>

#include <Eigen/Dense>

#include "conex/debug_macros.h"
#include "conex/memory_utils.h"

namespace conex {

using Clique = std::vector<int>;

struct TriangularMatrixWorkspace {
  TriangularMatrixWorkspace(const std::vector<Clique>& path_,
                            const std::vector<int>& supernode_size_)
      : supernode_size(supernode_size_) {

    N = std::accumulate(supernode_size.begin(), supernode_size.end(), 0);
    variable_to_supernode_.resize(N);
    variable_to_supernode_position_.resize(N);

    separators.resize(path_.size());
    int cnt = 0;
    for (auto& si : separators) {
      for (size_t i = supernode_size.at(cnt); i < path_.at(cnt).size(); i++) {
        si.push_back(path_.at(cnt).at(i));
      }
      cnt++;
    }

    cnt = 0;
    int var = 0;
    snodes.resize(path_.size());
    for (auto& si : snodes) {
      si.resize(supernode_size.at(cnt));
      for (int i = 0; i < supernode_size.at(cnt); i++) {
        if (var >= N) {
          std::runtime_error("Invalid variable index.");  
        }
        si.at(i) = path_.at(cnt).at(i);
        variable_to_supernode_[var] = cnt;
        variable_to_supernode_position_[var] = i;
        var++;
      }
      cnt++;
    }

  }
  int N;
  // TODO(FrankPermenter): Remove all of these members.
  std::vector<int> supernode_size;
  std::vector<Eigen::Map<Eigen::MatrixXd, Eigen::Aligned>> diagonal;
  std::vector<Eigen::Map<Eigen::MatrixXd, Eigen::Aligned>> off_diagonal;
  std::vector<std::vector<double*>> seperator_diagonal;

  std::vector<std::vector<int>> snodes;
  std::vector<std::vector<int>> separators;

  int SizeOfSupernode(int i) const {
    return get_size_aligned(supernode_size.at(i) * supernode_size.at(i));
  }

  int SizeOfSeparator(int i) const {
    return get_size_aligned(supernode_size.at(i) * separators.at(i).size());
  }

  friend int SizeOf(const TriangularMatrixWorkspace& o) {
    int size = 0;
    for (size_t j = 0; j < o.snodes.size(); j++) {
      size += o.SizeOfSupernode(j);
    }
    for (size_t j = 0; j < o.separators.size(); j++) {
      size += o.SizeOfSeparator(j);
    }
    return size;
  }

  friend void Initialize(TriangularMatrixWorkspace* o, double* data_start) {
    double* data = data_start;
    for (size_t j = 0; j < o->snodes.size(); j++) {
      o->diagonal.emplace_back(data, o->supernode_size.at(j),
                               o->supernode_size.at(j));

      data += o->SizeOfSupernode(j);
      o->off_diagonal.emplace_back(data, o->supernode_size.at(j),
                                   o->separators.at(j).size());
      data += o->SizeOfSeparator(j);
    }

    o->seperator_diagonal.resize(o->snodes.size());
    for (size_t j = 0; j < o->snodes.size(); j++) {
      o->S_S(j, &o->seperator_diagonal.at(j));
    }
    o->SetIntersections();

    // Use reserve so that we can call default constructor of LLT objects.
    o->llts.reserve(o->snodes.size());

    o->temporaries.resize(o->snodes.size());
    for (size_t j = 0; j < o->snodes.size(); j++) {
      o->temporaries.at(j).resize(o->separators.at(j).size());
    }
  }

  // Find the points in both separator(j) and supernode(i) for i + 1 > j.
  // For each point, returns the position in the separator and the
  // position in the supernode.
  std::vector<std::pair<int, int>> IntersectionOfSupernodeAndSeparator(
      int supernode, int separator) const;
  // A cache of IntersectionOfSupernodeAndSeparator. The first records
  // the j for which IntersectionOfSupernodeAndSeparator(i+1, j) is nonempty.
  // The second returns the output.
  std::vector<std::vector<int>> column_intersections;
  std::vector<std::vector<std::vector<std::pair<int, int>>>>
      intersection_position;

  std::vector<Eigen::LLT<Eigen::Ref<Eigen::MatrixXd>>> llts;

  // Needed for solving linear systems.
  mutable std::vector<Eigen::VectorXd> temporaries;

  std::vector<int> variable_to_supernode_;
  std::vector<int> variable_to_supernode_position_;
 private:
  void SetIntersections();
  // TODO(FrankPermenter): Remove this method.
  void S_S(int clique, std::vector<double*>*);

};

}  // namespace conex
