#include <vector>
#include <string>
#include <iostream>
#include <functional>

typedef unsigned int Uint32;

typedef int JobID;

int global = 0;

void updateA() {
    global += 10;
}

void updateB() {
    global *= 2;
}

void updateC() {
    global -= 50;
}

struct Job;

struct JobNode {
    JobID job;
    std::vector<JobNode> dependencies;
    std::vector<JobNode> users;
};

JobNode EmptyNode = {
    -1,
    {},
    {}
};

int idCounter = 0;

struct Job {
    std::vector<JobID> runAfter;
    std::vector<JobID> runBefore;
    std::function<void()> update;
    JobID id;
    std::string name;

    Job(std::function<void()> update, std::string name) {
        this->update = update;
        id = idCounter;
        this->name = name;
    }
};

Job* jobs;

JobNode jobToNode(Job& job) {
    JobNode node;
    node.job = job.id;
    for (auto jobId : job.runAfter) {
        JobNode dependentNode = jobToNode(jobs[jobId]);
        node.dependencies.push_back(dependentNode);
    }
    
    return node;
}

struct JobGraph {
    std::vector<JobNode> roots;
    JobNode* jobNodeMap;
    int numJobs;

    JobGraph(int numberOfJobs) {
        numJobs = numberOfJobs;
        jobNodeMap = (JobNode*)malloc(numJobs * sizeof(JobNode));
        for (int i = 0; i < numJobs; i++) {
            jobNodeMap[i] = EmptyNode;
        }
    }

    void jobRunsAfter(JobID a, JobID b) {
        jobs[a].runAfter.push_back(b);
        jobs[b].runBefore.push_back(a);
    }

    std::vector<Job> sequentialSchedule() {
        std::vector<Job> schedule;
        for (auto root : roots) {
            schedule.push_back(jobs[root.job]);
            jobNodeMap[root.job] = root;
        }

        for (auto root : roots) {
            for (auto user : root.users) {
                if (jobNodeMap[user.job].job == -1) {
                    schedule.push_back(jobs[root.job]);
                    jobNodeMap[user.job] = user;
                }
            }
        }

        return schedule;
    }
};

void addJob(Job job) {
    jobs[job.id] = job;
}

int main() {
    int nJobs = 3;
    jobs = (Job*)malloc(nJobs * sizeof(Job));
    JobGraph graph(nJobs);

    Job A(updateA, "A");
    idCounter++;
    Job B(updateB, "B");
    idCounter++;
    Job C(updateC, "C");
    idCounter++;

    addJob(A);
    addJob(B);
    addJob(C);

    for (int i = 0; i < nJobs; i++) {
        Job& job = jobs[i];
        if (job.runAfter.size() == 0)
            graph.roots.push_back(jobToNode(job));
    }

    std::vector<Job> jobSchedule = graph.sequentialSchedule();

    for (auto job : jobSchedule) {
        std::cout << "job " << job.name << "\n";
    }

    return 0;
}