#!/usr/bin/env python3
"""
Creates the necessary directory structure and files to run the testing of the
new objectives setup.
"""
import os
import argparse
import subprocess
from pathlib import Path

from modules.job_script import JobScript


def create_flow_set(flowset_generator_path: str, input_lgf_path: str, output_lgf_path: str, num_flow,
                    network_load: str):
    """Generate an LGF file with the specified details"""
    command = ['python', flowset_generator_path,
               '--input_lgf_path', input_lgf_path,
               '--output_lgf_path', output_lgf_path,
               '--num_flows', str(num_flow),
               '--scaling_factor', '30',
               '--flow_protocol', 'T',
               '--network_load', network_load]

    print(repr(command))

    process_status = subprocess.run(command)
    if process_status.returncode != 0:
        raise RuntimeError('The following command returned with errors\n{}'.format(command))

def create_lp_jobscript(net_load, num_flows, base_path, shell_graph_path, shell_base_path):
    """Creates the LP jobscript from the given parameters"""
    job_queue = 'short'
    job_name = 'lp_{}l_f_{}'.format(net_load[0], num_flows)
    job_logs_dir = '${HOME}/logs/'  # Nyquist logs directory
    job_not_flags = 'a'
    job_email = 'noel.farrugia.09@um.edu.mt'
    lp_job = JobScript(job_queue, job_name, job_logs_dir, job_not_flags, job_email)

    lp_result_file = shell_base_path + '/result_lp.xml'
    command = ('${{HOME}}/Development/LpSolver/build/release/lp-solver \\\n'
               '    --lgfPath {} \\\n'
               '    --xmlLogPath {} \\\n'
               '    --solverConfig mfmc'.format(shell_graph_path, lp_result_file))
    print(repr(command))
    lp_job.insert_command(command)

    lp_job_path = base_path + '/run_lp.pbs'
    print('Saving Lp job in {}'.format(lp_job_path))
    lp_job.save_job_script(lp_job_path)

def create_ksp_jobscript(k_value, net_load, num_flows, base_path, shell_graph_path, shell_base_path):
    """Creates the KSP command and jobscript from the given parameters.

    :return: The KSP result file path in shell format. This will be used when creating jobscripts dependent on the
             KSP file.
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

def create_pc_lp_jobscript(net_load, num_flows, ksp_result_file, base_path, shell_base_path):
    """Creates the Path Constrained LP jobscript from the given parameters"""
    job_queue = 'short'
    job_name = 'pc_lp_{}l_f_{}'.format(net_load[0], num_flows)
    job_logs_dir = '${HOME}/logs/'  # Nyquist logs directory
    job_not_flags = 'a'
    job_email = 'noel.farrugia.09@um.edu.mt'
    pc_lp_job = JobScript(job_queue, job_name, job_logs_dir, job_not_flags, job_email)

    pc_lp_result_file = shell_base_path + '/result_pc_lp.xml'
    command = ('${{HOME}}/Development/Development/PathConstrainedMCFPSolver/build/release/pc_mcfp_solver \\\n'
               '    -i {} \\\n'
               '    -o {}'.format(ksp_result_file, pc_lp_result_file))

    print(repr(command))
    pc_lp_job.insert_command(command)

    pc_lp_job_path = base_path + '/run_pc_lp.pbs'
    print('Saving Lp job in {}'.format(pc_lp_job_path))
    pc_lp_job.save_job_script(pc_lp_job_path)

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
               '    "delay_met, 1, _calculate_delay_distribution_metric, _get_delay_distribution_upper_bound " \\\n'
               '    "tot_flow_splits, -1, _calculate_flow_splits_metric, _get_flow_splits_upper_bound"'
               .format(ksp_result_file, ga_result_file))

    print(repr(command))
    ga_job.insert_command(command)

    ga_job_path = base_path + '/run_ga.pbs'
    print('Saving GA job in {}'.format(ga_job_path))
    ga_job.save_job_script(ga_job_path)


def main():
    # *Note* shell_<name> variables represent directories that are meant to be used by
    # the shell. These are used when writing paths to the jobscript command where the
    # path will be interpreted by the shell not by the python interpreter.

    parser = argparse.ArgumentParser()
    parser.add_argument('--base_dir', type=str, required=True,
                        help='The path to the base scratchpad directory')
    parser.add_argument('--shell_sp_dir', type=str, required=True,
                        help='The directory to the sp folder to be used by the Linux shell')
    parser.add_argument('--flowset_generator_path', type=str, required=True,
                        help='The path to the flow script generator')
    parser.add_argument('--input_lgf_path', type=str, required=True,
                        help='The path to the base lgf file to be used')
    parser.add_argument('--network_load', type=str, required=True, nargs='*', default=[],
                        help='The network loads to generate')
    parser.add_argument('--k_values', type=str, required=True, nargs='*', default=[],
                        help='The K values to use for each flow set')
    args = parser.parse_args()

    # Create the directories
    for net_load in args.network_load:
        if net_load == 'high':  # The max num of flows under high load is 150
            num_flows = list(range(50, 151, 50))
        else:
            num_flows = list(range(50, 301, 50))

        for num_flow in num_flows:
            current_dir = args.base_dir + '/load_' + net_load + '/flows_' + str(num_flow)
            shell_current_dir = args.shell_sp_dir + '/load_' + net_load + '/flows_' + str(num_flow)

            print('Creating directory {}'.format(current_dir))
            os.makedirs(current_dir, exist_ok=True)

            output_lgf_path = current_dir + '/graph.lgf'
            shell_graph_path = shell_current_dir + '/graph.lgf'

            # Create the graphs with the desired flow distribution
            create_flow_set(args.flowset_generator_path, args.input_lgf_path, output_lgf_path, num_flow, net_load)

            # Create Lp Solver job script
            create_lp_jobscript(net_load, num_flow, current_dir, shell_graph_path, shell_current_dir)

            # Create the KSP, Path Constrained LP & GA jobscripts
            for k_value in args.k_values:
                ksp_res_file = create_ksp_jobscript(k_value, net_load, num_flow, current_dir, shell_graph_path,
                                                    shell_current_dir)

                create_pc_lp_jobscript(net_load, num_flow, ksp_res_file, current_dir, shell_current_dir)

                create_ga_jobscript(k_value, net_load, num_flow, ksp_res_file, current_dir, shell_current_dir)


if __name__ == '__main__':
    main()
