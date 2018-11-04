import sys


class JobScript:
    def __init__(self, p_queue, p_job_name, p_logs_dir, p_notifications_flag="", p_email_address=""):
        self.job_name = p_job_name
        self.job_script_text = []
        self.__insert_bang()
        self.__insert_queue(p_queue)
        self.__insert_job_name()
        self.__insert_num_processing_cores_per_instance()
        if p_notifications_flag != "":
            self.__insert_notifications_and_email(p_notifications_flag, p_email_address)
        self.__insert_logs_dir(p_logs_dir)

    def insert_command(self, p_command):
        self.job_script_text.append(p_command)

    def save_job_script(self, js_path):
        job_script_file = open(js_path, "w")
        for line in self.job_script_text:
            job_script_file.write("%s" % line)

    def __insert_bang(self):
        bang_str = "#!/usr/bin/env bash\n#\n"
        self.job_script_text.append(bang_str)

    def __insert_queue(self, p_queue):
        self.job_script_text.append("#PBS -q " + p_queue + "\n#\n")

    def __insert_job_name(self):
        self.job_script_text.append("#PBS -N " + self.job_name + "\n#\n")

    def __insert_num_processing_cores_per_instance(self, p_num_cores=1):
        self.job_script_text.append("#PBS -l ncpus=" + str(p_num_cores) + "\n#\n")

    def __insert_notifications_and_email(self, p_notifications_flag, p_email_address):
        if p_notifications_flag != "" and p_email_address == "":
            print("You need an email address if you need to enable notifications")
            sys.exit()

        self.job_script_text.append("#PBS -m " + p_notifications_flag + "\n#\n")
        self.job_script_text.append("#PBS -M " + p_email_address + "\n#\n")

    def __insert_logs_dir(self, p_logs_dir):
        self.job_script_text.append("#PBS -e localhost:" + p_logs_dir + "${PBS_JOBNAME}.err.txt\n")
        self.job_script_text.append("#PBS -o localhost:" + p_logs_dir + "${PBS_JOBNAME}.out.txt\n\n")
