"""
Contains the necessary functions to convert the optimal solution given by the
Genetic algorithm into a format that is understandable to ns3.
"""
import os
import sys
import math
import argparse
from pathlib import Path
from lxml import etree
import numpy as np
import pandas

script_path = os.path.dirname(os.path.abspath( __file__ ))
sys.path.append(script_path + '/../modules/')

from flow import Flow


def parse_population_to_pandas(flows, last_population, merged_xml_path):
    """
    Parse the last population and stores the information in a pandas data frame

    :param flows: Flows dictionary
    :param last_population: The last population
    """
    results_df = pandas.DataFrame()

    k = find_k_value(merged_xml_path)

    for soln_id, chromosome in enumerate(last_population):
        # Holds the number of flows that use the number of paths based on the
        # list index value. K + 1 is used because 0 is included.
        paths_used = [0] * (k + 1)

        for _, flow in flows.items():
            # Exclude ACK flows
            if flow.protocol == 'A':
                continue

            # Count the number of paths that are being used by the flow
            n_paths_used = len([chromosome[path_id] for path_id in flow.paths
                                if chromosome[path_id] > ACCURACY_VALUE])

            paths_used[n_paths_used] += 1

        chromosome_data_point = {'NetFlow': chromosome.network_flow,
                                 'NetCost': chromosome.network_cost,
                                 'Paths': chromosome.paths_used,
                                 'PopulationIndex': soln_id}

        # Add the number of flows that are using that number of paths
        for idx, n_paths in enumerate(paths_used):
            chromosome_data_point[str(idx)] = n_paths

        results_df = results_df.append(chromosome_data_point,
                                       ignore_index=True)

    sort_type = list(range(1, k+1))
    sort_type = list(map(str, sort_type))
    sort_type.append('NetFlow')
    sort_type.append('NetCost')

    # Sort all values in descedning order except the Cost, we want the smallest
    # value for that.
    ascending_sort = [False] * len(sort_type)
    ascending_sort[-1] = True
    results_df.sort_values(sort_type, ascending=ascending_sort,
                           inplace=True)

    results_df.reset_index(drop=True, inplace=True)
    return results_df


def select_min_flow_splits(flows, last_population, merged_xml_path, plot_root,
                           csv_path):
    results_df = parse_population_to_pandas(flows, last_population,
                                            merged_xml_path)

    if csv_path:  # Export results to csv file
        results_df.to_csv(csv_path)

    min_flow_splits_soln = \
        last_population[int(results_df['PopulationIndex'].iloc[0])]

    min_flow_splits_element = etree.SubElement(plot_root, 'MinFlowSplits')
    min_flow_splits_element.set('flow', str(min_flow_splits_soln.network_flow))
    min_flow_splits_element.set('cost', str(min_flow_splits_soln.network_cost))
    min_flow_splits_element.set('paths', str(min_flow_splits_soln.paths_used))

    return min_flow_splits_soln


def select_min_paths(flows, last_population, plot_root):
    """
    Choose the solution with the minimum number of paths, max flow and minimum
    cost, GIVEN that all the flows are allocated something.
    """

    # contains all the solutions where every flow is allocated something
    fit_solutions = list()

    for chromosome in last_population:
        all_flows_allocated_data = True

        for _, flow in flows.items():  # Loop through all flows, excluding ACKs
            if flow.protocol != 'A':
                flow_allocated_data = False
                for path_id in flow.paths:  # Loop through all paths
                    if chromosome[path_id] >= ACCURACY_VALUE:
                        flow_allocated_data = True
                        break

                # Break the loop if a flow is not allocated data
                if flow_allocated_data is False:
                    all_flows_allocated_data = False
                    break

        if all_flows_allocated_data is True:
            fit_solutions.append(chromosome)

    if not fit_solutions:  # Return none if no fit solutions were found
        return None

    export_population_to_xml(fit_solutions, 'FitSolutions', plot_root)

    # Choose the paths with the smallest number of paths, max flow, min cost
    max_flow = 0
    min_cost = math.inf
    min_paths = math.inf

    best_solution_idx = 0

    for idx, chromosome in enumerate(fit_solutions):
        flow = chromosome.network_flow
        cost = chromosome.network_cost
        paths = chromosome.paths_used

        if paths < min_paths:
            best_solution_idx = idx
            max_flow = flow
            min_cost = cost
            min_paths = paths
        elif paths == min_paths:
            if flow > max_flow:
                best_solution_idx = idx
                max_flow = flow
                min_cost = cost
                min_paths = paths
            elif flow == max_flow:
                if cost < min_cost:
                    best_solution_idx = idx
                    max_flow = flow
                    min_cost = cost
                    min_paths = paths

    min_paths_soln = fit_solutions[best_solution_idx]

    min_paths_soln_element = etree.SubElement(plot_root, 'MinPaths')
    min_paths_soln_element.set('flow', str(min_paths_soln.network_flow))
    min_paths_soln_element.set('cost', str(min_paths_soln.network_cost))
    min_paths_soln_element.set('paths', str(min_paths_soln.paths_used))

    return min_paths_soln


def add_optimal_solution(best_solution, root, act_net_mat, network):
    """
    Add the OptimalSolution element in the XML file used by ns3 to run
    simulations
    """
    optimal_soln_element = etree.SubElement(root, 'OptimalSolution')
    comment = ('DataRate (Mbps), PacketSize (bytes), Protocol (U=UDP,T=TCP), '
               'Time (Seconds).\nUnless Specified the port number refers to '
               'the destination port number')
    optimal_soln_element.append(etree.Comment(comment))

    for flow_id, flow in network.flows.items():
        flow_element = etree.SubElement(optimal_soln_element, 'Flow')
        flow_element.set('Id', str(flow_id))
        flow_element.set('SourceNode', str(flow.src_node))
        flow_element.set('DestinationNode', str(flow.dst_node))

        if flow.protocol == 'T':
            flow_element.set('SrcPortNumber', str(flow.src_port))
            flow_element.set('DstPortNumber', str(flow.dst_port))
        else:
            flow_element.set('PortNumber', str(flow.dst_port))

        allocated_data_rate = sum([best_solution[path_id] for path_id in
                                   flow.paths])
        flow_element.set('DataRate', str(allocated_data_rate))
        flow_element.set('RequestedDataRate', str(flow.requested_rate))
        flow_element.set('PacketSize', str(flow.pkt_size))
        flow_element.set('NumOfPackets', str(flow.num_pkts))
        flow_element.set('Protocol', str(flow.protocol))
        flow_element.set('StartTime', str(flow.start_time))
        flow_element.set('EndTime', str(flow.end_time))

        flow_matrix = list()
        for path_id in flow.paths:
            flow_matrix.append(act_net_mat[path_id, :])

        flow_matrix = np.array(flow_matrix)
        total_link_use = dict()

        # Iterate through the columns
        for link_id in range(0, flow_matrix.shape[1]):
            link_usage = np.sum(flow_matrix[:, link_id])
            if abs(link_usage) < ACCURACY_VALUE:
                link_usage = 0

            assert link_usage >= 0, \
                'Negative link usage found. Value: {}'.format(link_usage)

            if link_usage > 0:  # Add the link usage
                if link_id in total_link_use:
                    total_link_use[link_id] += link_usage
                else:
                    total_link_use[link_id] = link_usage

        for link_id, link_use in sorted(total_link_use.items()):
            link_element = etree.SubElement(flow_element, 'Link')
            link_element.set('Id', str(link_id))
            link_element.set('FlowRate', str(link_use))


def add_incoming_flow(root, act_net_mat, network):
    # Key -> Node Id Value -> List of incoming links of the node
    incoming_links = dict()

    for link in root.findall('NetworkTopology/Link'):
        for link_element in link.findall('LinkElement'):
            dst_node = int(link_element.get('DestinationNode'))
            link_id = int(link_element.get('Id'))

            if dst_node in incoming_links:
                incoming_links[dst_node].append(link_id)
            else:
                incoming_links[dst_node] = [link_id]

    incoming_flow_element = etree.SubElement(root, 'IncomingFlow')

    for node_id, links in sorted(incoming_links.items()):
        node_element = etree.SubElement(incoming_flow_element, 'Node')
        node_element.set('Id', str(node_id))

        flow_data_rate = dict()  # Key -> Flow Id Value -> Data Rate
        for link in links:
            link_column = act_net_mat[:, link]

            for path_id, usage in enumerate(link_column):
                if usage > 0:
                    flow_id = network.paths[path_id].flow_id

                    if flow_id in flow_data_rate:
                        flow_data_rate[flow_id] += usage
                    else:
                        flow_data_rate[flow_id] = usage

        for flow_id, data_rate in flow_data_rate.items():
            flow_element = etree.SubElement(node_element, 'Flow')
            flow_element.set('Id', str(flow_id))
            flow_element.set('IncomingFlow', str(data_rate))


def merge_xml_files(ksp_xml_path, ksp_ack_xml_path, merged_xml_path):
    """
    Merge the TCP data file with the ACK data file. This new merged file will
    be used to build the Network class and the matrix.

    :param ksp_xml_path:     Path to the KSP TCP ACK flows file
    :param ksp_ack_xml_path: Path to the KSP TCP Data flows file
    """
    parser = etree.XMLParser(remove_blank_text=True)
    root_data = etree.parse(ksp_xml_path, parser).getroot()
    root_ack = etree.parse(ksp_ack_xml_path, parser).getroot()

    flow_details_element = root_data.find('FlowDetails')

    for flow in root_ack.findall('FlowDetails/Flow'):
        flow_details_element.append(flow)

    num_flows_in_ack = int(root_ack.find('FlowDetails').get('NumFlows'))
    num_paths_in_ack = int(root_ack.find('FlowDetails').get('NumPaths'))

    num_flows_in_data = int(root_data.find('FlowDetails').get('NumFlows'))
    num_paths_in_data = int(root_data.find('FlowDetails').get('NumPaths'))

    flow_details_element.set('NumFlows', str(num_flows_in_ack +
                                             num_flows_in_data))
    flow_details_element.set('NumPaths', str(num_paths_in_ack +
                                             num_paths_in_data))

    tree = etree.ElementTree(root_data)
    tree.write(merged_xml_path, pretty_print=True)


def add_ack_flows(merged_xml_path, best_solution, network):
    """
    Adds the acknowledgement paths in the best_solution chromosome. The
    acknowledgement flows are routed over the path with least cost.

    :param merged_xml_path: The merged xml path that contains both data and ack
                            flows
    :param best_solution:   The best solution. This contains only the data
                            flows
    :returns:               The updated best solution including the
                            acknowledgement flows
    """
    parser = etree.XMLParser(remove_blank_text=True)
    root = etree.parse(merged_xml_path, parser).getroot()

    flow_details_element = root.find('FlowDetails')
    num_paths = int(flow_details_element.get('NumPaths'))

    additional_paths = [0] * (num_paths - len(best_solution))
    best_solution.extend(additional_paths)

    for _, flow in network.flows.items():
        # Route only ACK flows
        if flow.protocol != 'A':
            continue

        # Find the path with the smallest cost
        path_details = [(path_id, network.paths[path_id].cost)
                        for path_id in flow.paths]
        min_cost_path_id = min(path_details, key=lambda t: t[1])[0]
        best_solution[min_cost_path_id] = flow.requested_rate

    return best_solution


def find_k_value(merged_xml_path):
    """
    Loops through all the flow's paths to determine the largest number of
    paths. This is equivalent to the K value set by the KSP algorithm.
    """
    parser = etree.XMLParser(remove_blank_text=True)
    root = etree.parse(merged_xml_path, parser).getroot()

    k = 0
    for flow in root.findall('FlowDetails/Flow'):
        paths_element = flow.find('Paths')
        k = max(int(paths_element.get('NumPaths')), k)

    return k


def main():
    """
    Converts the Genetic Algorithm result to something that is understood by
    the ns3 simulator.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('--ksp_xml_file', required=True,
                        help='The path to the KSP XML file')
    parser.add_argument('--ga_result_file', required=True,
                        help='The path to the GA result XML file.')
    parser.add_argument('--ksp_ns3_log_file', required=True,
                        help='The location of the ns3 log file generated by '
                             'the KSP algorithm')
    parser.add_argument('--output', required=True,
                        help='The location where to store the generated XML file')
    args = parser.parse_args()

    # Parse the KSP XML file and build the flows
    parser = etree.XMLParser(remove_blank_text=True)
    print('Parsing the KSP Xml file in {}'.format(args.ksp_xml_file))
    ksp_xml_root = etree.parse(args.ksp_xml_file).getroot()

    flows = Flow.parse_flows(ksp_xml_root)

    # NOTE: Continue from here!
    # FIXME: Append the ACK flows to the flows dictionary so they are treated as all the others

    # Retrieve the last population
    ga_xml_root = etree.parse(args.ga_result_file).getroot()

    num_generations = int(ga_xml_root.find('Parameters').get('num_generations'))
    last_pop = ga_xml_root.xpath("//Generation[@number='{}']/Chromosome".format(num_generations))

    # Find the MaxFlow solution with no splits
    chromosome_index = -1
    max_flow = 0

    for idx, chromosome in enumerate(last_pop):
        net_flow = float(chromosome.get('net_flow'))
        tot_flow_splits = float(chromosome.get('tot_flow_splits'))
        if tot_flow_splits == 0 and net_flow > max_flow:
            max_flow = net_flow
            chromosome_index = idx

    assert chromosome_index > 0, 'No solution with no flow splits found'

    # Key: path id | Value: data rate on path
    best_solution = {int(gene.get('path_id')) : float(gene.get('value'))
                     for gene in last_pop[chromosome_index]}

    # Create XML document here
    root_element = etree.Element('Log')

    # # # Create Optimal Solution Element # # #
    optimal_soln_element = etree.SubElement(root_element, 'OptimalSolution')
    comment = ('DataRate (Mbps), PacketSize (bytes), Protocol (U=UDP,T=TCP), '
               'Time (Seconds).\nUnless Specified the port number refers to '
               'the destination port number')
    optimal_soln_element.append(etree.Comment(comment))

    for flow_id in sorted(flows):
        flow = flows[flow_id]
        flow_link_usage = dict()  # Key: link id | Value: total data rate on link
        flow_paths = flow.get_paths()
        for path in flow_paths:
            dr_on_path = best_solution[path.id]

            if dr_on_path > 0:
                for link_id in path.links:
                    if link_id in flow_link_usage:
                        flow_link_usage[link_id] += dr_on_path
                    else:
                        flow_link_usage[link_id] = dr_on_path

        flow_element = etree.SubElement(optimal_soln_element, 'Flow')
        flow_element.set('Id', str(flow.id))
        flow_element.set('SourceNode', str(flow.src_node))
        flow_element.set('DestinationNode', str(flow.dst_node))

        if flow.protocol == 'T':
            flow_element.set('SrcPortNumber', str(flow.src_port))
            flow_element.set('DstPortNumber', str(flow.dst_port))
        else:
            flow_element.set('PortNumber', str(flow.dst_port))

        allocated_data_rate = sum([best_solution[path.id] for path in flow_paths])
        flow_element.set('DataRate', str(allocated_data_rate))
        flow_element.set('RequestedDataRate', str(flow.requested_rate))
        flow_element.set('PacketSize', str(flow.pkt_size))
        flow_element.set('NumOfPackets', str(flow.num_packets))
        flow_element.set('Protocol', str(flow.protocol))
        flow_element.set('StartTime', str(flow.start_time))
        flow_element.set('EndTime', str(flow.end_time))

        for link_id in sorted(flow_link_usage.keys()):
            link_element = etree.SubElement(flow_element, 'Link')
            link_element.set('Id', str(link_id))
            link_element.set('FlowRate', str(flow_link_usage[link_id]))

    # # # Add incoming flow section # # #
    ksp_ns3_log_root = etree.parse(args.ksp_ns3_log_file).getroot()

    node_incoming_links = dict()  # Key -> Node Id Value -> List of incoming links of the node
    for link in ksp_ns3_log_root.findall('NetworkTopology/Link'):  # FIXME use the ns3log xml file here!
        for link_element in link.findall('LinkElement'):
            dst_node = int(link_element.get('DestinationNode'))
            link_id = int(link_element.get('Id'))

            if dst_node in node_incoming_links:
                node_incoming_links[dst_node].append(link_id)
            else:
                node_incoming_links[dst_node] = [link_id]

    incoming_flow_element = etree.SubElement(root_element, 'IncomingFlow')

    for node_id, links in sorted(node_incoming_links.items()):
        node_element = etree.SubElement(incoming_flow_element, 'Node')
        node_element.set('Id', str(node_id))

        flow_rate = dict()  # Key: Flow Id | Value: Incoming flow

        for flow_id in sorted(flows.keys()):
            flow = flows[flow_id]

            for path in flow.get_paths():
                if best_solution[path.id] > 0:
                    for link in path.links:
                        if link in links:  # Link exists
                            if flow_id in flow_rate:
                                flow_rate[flow_id] += best_solution[path.id]  # Get the data rate on that path
                            else:
                                flow_rate[flow_id] = best_solution[path.id]  # Get the data rate on that path

        for flow_id, incoming_flow in flow_rate.items():
            flow_element = etree.SubElement(node_element, 'Flow')
            flow_element.set('Id', str(flow_id))
            flow_element.set('IncomingFlow', str(incoming_flow))

    # Save XML file
    tree = etree.ElementTree(root_element)
    with open(args.output, 'wb') as output_file:
        tree.write(output_file, pretty_print=True, xml_declaration=True, encoding='utf-8')


if __name__ == "__main__":
    main()
