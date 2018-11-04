#!/usr/bin/env python3
"""
Creates the necessary directory structure and files to run the testing of the
new objectives setup.
"""
import os
import subprocess
from pathlib import Path

from modules.job_script import JobScript


def create_flow_set(out_lgf_path, num_flow, net_load):
    """Create a graph file with the specified details."""
    home_dir = str(Path.home())
    dev_dir = home_dir + '/Development/Development/SimulationSetup'
    command = ['python3',
               dev_dir + '/generate_random_flow_set.py',
               '--base_lgf_file', dev_dir + '/base_geant.lgf',
               '--save_lgf_file', out_lgf_path,
               '--num_flows', str(num_flow),
               '--scaling_factor', '30',
               '--flow_protocol', 'T',
               '--vid_res_prob']

    # Vid flow resolution order: SD, HD, UHD
    if net_load == 'high':
        command.extend(['0.0', '0.0', '1.0'])
    elif net_load == 'medium':
        command.extend(['0.0', '1.0', '0.0'])
    elif net_load == 'low':
        command.extend(['1.0', '0.0', '0.0'])
    else:
        raise RuntimeError('The following network load is unknown {}'.format(net_load))

    print(repr(command))

    process_status = subprocess.run(command)
    if process_status.returncode != 0:
        raise RuntimeError('The following command returned with errors\n{}'.format(command))


def create_ksp_jobscript(k_value, net_load, num_flows, base_path, shell_graph_path, shell_base_path):
    """Creates the KSP command and jobscript from the given parameters.

    :return: The KSP result file path in shell format. This will be used when creating the genetic
             algorithm jobscript.
    """
    job_queue = 'short'
    job_name = 'ksp_{}l_f_{}_k_{}'.format(net_load[0], num_flows, k_value)
    job_logs_dir = '${HOME}/logs/'  # Nyquist home directory
    job_not_flags = 'a'
    job_email = 'noel.farrugia.09@um.edu.mt'
    ksp_job = JobScript(job_queue, job_name, job_logs_dir, job_not_flags, job_email)

    ksp_result_file = shell_base_path + '/result_ksp_k_' + k_value + '.xml'

    command = ('${{HOME}}/Development/KShortestPath/build/release/ksp \\\n'
               '    --k {} \\\n'
               '    --lgfPath {} \\\n'
               '    --kspXmlPath {}'.format(k_value, shell_graph_path, ksp_result_file))

    print(repr(command))
    ksp_job.insert_command(command)

    ksp_job_path = base_path + '/run_ksp.pbs'
    print('Saving Ksp job in {}'.format(ksp_job_path))
    ksp_job.save_job_script(ksp_job_path)

    return ksp_result_file


def create_ga_jobscript(k_value, net_load, num_flows, ksp_result_file, base_path, shell_base_path):
    """Creates the GA command and jobscript from the given parameters."""
    job_queue = 'medium'
    job_name = 'ga_{}l_f_{}_k_{}'.format(net_load[0], num_flows, k_value)
    job_logs_dir = '${HOME}/logs/'  # Nyquist home directory
    job_not_flags = 'a'
    job_email = 'noel.farrugia.09@um.edu.mt'
    ga_job = JobScript(job_queue, job_name, job_logs_dir, job_not_flags, job_email)

    ga_result_file = shell_base_path + '/result_ga_k_' + k_value + '.xml'

    command = ('python3 ${{HOME}}/Development/Development/GeneticAlgorithm/ga.py \\\n'
               '    --ksp_xml_file {} \\\n'
               '    --result_file {} \\\n'
               '    --num_generations 400 \\\n'
               '    --pop_size 800 \\\n'
               '    --prob_crossover 0.9 \\\n'
               '    --prob_mutation 0.2 \\\n'
               '    --mutation_fraction 0.1 \\\n'
               '    --objectives \\\n'
               '    "net_flow, 1, _calculate_total_network_flow, _get_network_flow_upper_bound " \\\n'
               '    "net_cost, -1, _calculate_total_network_cost, _get_network_cost_upper_bound " \\\n'
               '    "tot_flow_splits, -1, _calculate_flow_splits_metric, _get_flow_splits_upper_bound"'
               .format(ksp_result_file, ga_result_file))

    print(repr(command))
    ga_job.insert_command(command)

    ga_job_path = base_path + '/run_ga.pbs'
    print('Saving GA job in {}'.format(ga_job_path))
    ga_job.save_job_script(ga_job_path)


def main():
    # *Note* shell_<name> variables represent directories that are meant to be used by
    # the shell. This are used when writing paths to the jobscript command where the
    # path will be interpreted by the shell not by the python interpreter.

    home_dir = str(Path.home())
    sp_objs_dir = 'GDrive/Scratchpad/genetic_algorithm/new_objectives/flow_splits_only'
    base_directory = str(Path.home()) + '/' + sp_objs_dir
    network_load = ['high', 'medium', 'low']
    k_values = ['5']

    # Create the directories
    for net_load in network_load:
        if net_load == 'high':  # The max num of flows under high load is 150
            num_flows = list(range(50, 151, 50))
        else:
            num_flows = list(range(50, 301, 50))

        for num_flow in num_flows:
            shell_base_dir = '${HOME}/' + sp_objs_dir  # Used when writing the path to be used by a shell
            curr_dir = base_directory + '/load_' + net_load + '/flows_' + str(num_flow)
            shell_curr_dir = shell_base_dir + '/load_' + net_load + '/flows_' + str(num_flow)

            print('Creating directory {}'.format(curr_dir))
            os.makedirs(curr_dir)

            graph_path = curr_dir + '/graph.lgf'
            shell_graph_path = shell_curr_dir + '/graph.lgf'

            # Create the graphs with the desired flow distribution
            create_flow_set(graph_path, num_flow, net_load)

            # Create the KSP jobscripts
            for k_value in k_values:
                ksp_res_file = create_ksp_jobscript(k_value, net_load, num_flow, curr_dir,
                                                    shell_graph_path, shell_curr_dir)
                create_ga_jobscript(k_value, net_load, num_flow, ksp_res_file, curr_dir,
                                    shell_curr_dir)


if __name__ == '__main__':
    main()
