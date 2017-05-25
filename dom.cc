/* Copyright 2017 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dom.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>

#include "default_mapper.h"

using namespace Legion;
using namespace Legion::Mapping;

///
/// Mapper
///

static LegionRuntime::Logger::Category log_dom("dom");

class DOMMapper : public DefaultMapper
{
public:
  DOMMapper(MapperRuntime *rt, Machine machine, Processor local,
                const char *mapper_name,
                std::vector<Processor>* procs_list,
                std::vector<Memory>* sysmems_list,
                std::map<Memory, std::vector<Processor> >* sysmem_local_procs,
                std::map<Processor, Memory>* proc_sysmems,
                std::map<Processor, Memory>* proc_regmems);
  virtual Processor default_policy_select_initial_processor(
                                    MapperContext ctx, const Task &task);
  virtual void default_policy_select_target_processors(
                                    MapperContext ctx,
                                    const Task &task,
                                    std::vector<Processor> &target_procs);
  virtual LogicalRegion default_policy_select_instance_region(
                                  MapperContext ctx, Memory target_memory,
                                  const RegionRequirement &req,
                                  const LayoutConstraintSet &layout_constraints,
                                  bool force_new_instances,
                                  bool meets_constraints);
  virtual Memory default_policy_select_target_memory(MapperContext ctx,
                                                     Processor target_proc);

  //virtual void map_task(const MapperContext      ctx,
  //                      const Task&              task,
  //                      const MapTaskInput&      input,
  //                            MapTaskOutput&     output);
private:
  std::vector<Processor>& procs_list;
  // std::vector<Memory>& sysmems_list;
  std::map<Memory, std::vector<Processor> >& sysmem_local_procs;
  std::map<Processor, Memory>& proc_sysmems;
  // std::map<Processor, Memory>& proc_regmems;
};

DOMMapper::DOMMapper(MapperRuntime *rt, Machine machine, Processor local,
                             const char *mapper_name,
                             std::vector<Processor>* _procs_list,
                             std::vector<Memory>* _sysmems_list,
                             std::map<Memory, std::vector<Processor> >* _sysmem_local_procs,
                             std::map<Processor, Memory>* _proc_sysmems,
                             std::map<Processor, Memory>* _proc_regmems)
  : DefaultMapper(rt, machine, local, mapper_name),
    procs_list(*_procs_list),
    // sysmems_list(*_sysmems_list),
    sysmem_local_procs(*_sysmem_local_procs),
    proc_sysmems(*_proc_sysmems)// ,
    // proc_regmems(*_proc_regmems)
{
}

Processor DOMMapper::default_policy_select_initial_processor(
                                    MapperContext ctx, const Task &task)
{
  const char* task_name = task.get_task_name();
  if (strcmp(task_name, "initialize") == 0 ||
      strcmp(task_name, "source_term") == 0 ||
      strcmp(task_name, "west_bound") == 0 ||
      strcmp(task_name, "east_bound") == 0 ||
      strcmp(task_name, "south_bound") == 0 ||
      strcmp(task_name, "north_bound") == 0 ||
      strcmp(task_name, "sweep_1") == 0 ||
      strcmp(task_name, "sweep_2") == 0 ||
      strcmp(task_name, "sweep_3") == 0 ||
      strcmp(task_name, "sweep_4") == 0 ||
      strcmp(task_name, "residual") == 0 ||
      strcmp(task_name, "update") == 0)
  {
    if (procs_list.size() > 1 && task.regions[0].handle_type == SINGULAR) {
      size_t color = 0;
      const LogicalRegion& region = task.regions[0].region;
      if (runtime->has_parent_index_partition(ctx, region.get_index_space())) {
#define DIM 2
        IndexPartition ip =
          runtime->get_parent_index_partition(ctx, region.get_index_space());
        Domain domain =
          runtime->get_index_partition_color_space(ctx, ip);
        DomainPoint point =
          runtime->get_logical_region_color_point(ctx, region);
        coord_t size_x = domain.rect_data[DIM] - domain.rect_data[0] + 1;
        color = point.point_data[0] + point.point_data[1] * size_x;
      }
      return procs_list[color % procs_list.size()];
    } else {
      return local_proc;
    }
  }

  return DefaultMapper::default_policy_select_initial_processor(ctx, task);
}

void DOMMapper::default_policy_select_target_processors(
                                    MapperContext ctx,
                                    const Task &task,
                                    std::vector<Processor> &target_procs)
{
  target_procs.push_back(task.target_proc);
}

LogicalRegion DOMMapper::default_policy_select_instance_region(
                                MapperContext ctx, Memory target_memory,
                                const RegionRequirement &req,
                                const LayoutConstraintSet &layout_constraints,
                                bool force_new_instances,
                                bool meets_constraints)
{
  return req.region;
}

Memory DOMMapper::default_policy_select_target_memory(MapperContext ctx,
                                                      Processor target_proc)
{
  return proc_sysmems[target_proc];
}

//void DOMMapper::map_task(const MapperContext      ctx,
//                         const Task&              task,
//                         const MapTaskInput&      input,
//                               MapTaskOutput&     output)
//{
//  const char* task_name = task.get_task_name();
//  if (strcmp(task_name, "west_bound") == 0 ||
//      strcmp(task_name, "east_bound") == 0 ||
//      strcmp(task_name, "south_bound") == 0 ||
//      strcmp(task_name, "north_bound") == 0 ||
//      strcmp(task_name, "initialize_x_faces") == 0 ||
//      strcmp(task_name, "initialize_y_faces") == 0)
//  {
//    Processor::Kind target_kind = task.target_proc.kind();
//    VariantInfo chosen = default_find_preferred_variant(task, ctx,
//                      true/*needs tight bound*/, true/*cache*/, target_kind);
//    output.chosen_variant = chosen.variant;
//    output.task_priority = 0;
//    output.postmap_task = false;
//    default_policy_select_target_processors(ctx, task, output.target_procs);
//    assert(task.regions.size() <= 2);
//
//    Memory target_memory =
//      default_policy_select_target_memory(ctx, task.target_proc);
//
//    const TaskLayoutConstraintSet &layout_constraints =
//      runtime->find_task_layout_constraints(ctx,
//                          task.task_id, output.chosen_variant);
//
//    {
//      unsigned idx = 0;
//      const RegionRequirement& req = task.regions[idx];
//      std::vector<std::set<FieldID> > fields;
//      for (std::set<FieldID>::iterator it = req.privilege_fields.begin();
//           it != req.privilege_fields.end(); ++it)
//      {
//        std::set<FieldID> singleton;
//        singleton.insert(*it);
//        fields.push_back(singleton);
//      }
//
//      for (unsigned i = 0; i < fields.size(); ++i)
//        if (!default_create_custom_instances(ctx, task.target_proc,
//                target_memory, req, idx, fields[i], layout_constraints, true,
//                output.chosen_instances[idx]))
//        {
//          default_report_failed_instance_creation(task, idx,
//                                      task.target_proc, target_memory);
//        }
//    }
//    if (task.regions.size() > 1) {
//      unsigned idx = 1;
//      std::set<FieldID> fields(task.regions[idx].privilege_fields);
//      if (!default_create_custom_instances(ctx, task.target_proc,
//              target_memory, task.regions[idx], idx, fields,
//              layout_constraints, true,
//              output.chosen_instances[idx]))
//      {
//        default_report_failed_instance_creation(task, idx,
//                                    task.target_proc, target_memory);
//      }
//    }
//  }
//  else
//    DefaultMapper::map_task(ctx, task, input, output);
//}

static void create_mappers(Machine machine, HighLevelRuntime *runtime, const std::set<Processor> &local_procs)
{
  std::vector<Processor>* procs_list = new std::vector<Processor>();
  std::vector<Memory>* sysmems_list = new std::vector<Memory>();
  std::map<Memory, std::vector<Processor> >* sysmem_local_procs =
    new std::map<Memory, std::vector<Processor> >();
  std::map<Processor, Memory>* proc_sysmems = new std::map<Processor, Memory>();
  std::map<Processor, Memory>* proc_regmems = new std::map<Processor, Memory>();


  std::vector<Machine::ProcessorMemoryAffinity> proc_mem_affinities;
  machine.get_proc_mem_affinity(proc_mem_affinities);

  for (unsigned idx = 0; idx < proc_mem_affinities.size(); ++idx) {
    Machine::ProcessorMemoryAffinity& affinity = proc_mem_affinities[idx];
    if (affinity.p.kind() == Processor::LOC_PROC) {
      if (affinity.m.kind() == Memory::SYSTEM_MEM) {
        (*proc_sysmems)[affinity.p] = affinity.m;
        if (proc_regmems->find(affinity.p) == proc_regmems->end())
          (*proc_regmems)[affinity.p] = affinity.m;
      }
      else if (affinity.m.kind() == Memory::REGDMA_MEM)
        (*proc_regmems)[affinity.p] = affinity.m;
    }
  }

  for (std::map<Processor, Memory>::iterator it = proc_sysmems->begin();
       it != proc_sysmems->end(); ++it) {
    procs_list->push_back(it->first);
    (*sysmem_local_procs)[it->second].push_back(it->first);
  }

  for (std::map<Memory, std::vector<Processor> >::iterator it =
        sysmem_local_procs->begin(); it != sysmem_local_procs->end(); ++it)
    sysmems_list->push_back(it->first);

  for (std::set<Processor>::const_iterator it = local_procs.begin();
        it != local_procs.end(); it++)
  {
    DOMMapper* mapper = new DOMMapper(runtime->get_mapper_runtime(),
                                              machine, *it, "dom_mapper",
                                              procs_list,
                                              sysmems_list,
                                              sysmem_local_procs,
                                              proc_sysmems,
                                              proc_regmems);
    runtime->replace_default_mapper(mapper, *it);
  }
}

void register_mappers()
{
  HighLevelRuntime::set_registration_callback(create_mappers);
}
