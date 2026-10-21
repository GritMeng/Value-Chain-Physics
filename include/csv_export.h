#pragma once
#include <string>
#include <vector>
#include "ipc_types.h"

namespace ipc {

void csv_export_parts(const std::vector<PartSiteRecord>& parts);
void csv_export_boms(const std::vector<FlatBomItem>& boms);
void csv_export_demands(const std::vector<IndependentDemand>& demands);
void csv_export_ipc_planned_orders(const std::vector<PlannedOrder>& orders);
void csv_export_alternates(const std::vector<AlternateAllocationRecord>& alts);
void csv_export_lsctree(const std::vector<LscTreeNode>& nodes);

} // namespace ipc

