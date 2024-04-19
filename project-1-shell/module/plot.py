import igraph
from igraph import Graph, EdgeSeq
import re
import time
import json
import math
import matplotlib.pyplot as plt
import sys
import os

def main():
    tree=[]
    with open("tree.txt", "r") as f:
        lines = f.readlines()
        for line in lines:
            line = re.sub("([\[]).*?([\]])", "", line).strip(" ").strip("\n")
            if line[0] == "{":
                tree.append(json.loads(line))
    if len(tree) == 0:
        return 0
    nodes={}
    for node in tree:
        if node["pid"] not in list(nodes.values()):
            nodes[node["pid"]] = len(nodes)

    nodes_={}
    for k,v in nodes.items():
        nodes_[v] = k

    add_list = []
    for node in tree:
        if node["ppid"] != -1:
            add_list.append((nodes[node["ppid"]], nodes[node["pid"]]))
    nr_vertices = len(tree)
    G = Graph.Tree(nr_vertices, 1)
    for i in range(len(tree)):
        for j in range(len(tree)):
            try:
                G.delete_edges([(i,j)])
            except:
                continue
    G.add_edges(add_list)
    lay = G.layout('rt', root=[0])

    eldest_childs = {}
    start_times = {}
    for node in tree:
        eldest_childs[node["pid"]] = -1
        start_times[node["pid"]] = node["start_time"]

    for node in tree:
        if node["ppid"] != -1:
            cur_ed = eldest_childs[node["ppid"]]
            if cur_ed == -1:
                eldest_childs[node["ppid"]] = node["pid"]
            else:
                cur_st = start_times[node["ppid"]]
                if start_times[node["pid"]] < cur_st:
                    eldest_childs[node["ppid"]] = node["pid"]

    position = {k: lay[k] for k in range(nr_vertices)}
    Y = [lay[k][1] for k in range(nr_vertices)]
    M = max(Y)

    es = EdgeSeq(G) # sequence of edges
    E = [e.tuple for e in G.es] # list of edges

    L = len(position)
    Xn = [position[k][0] for k in range(L)]
    Yn = [2*M-position[k][1] for k in range(L)]
    Xe = []
    Ye = []
    for edge in E:
        Xe+=[position[edge[0]][0],position[edge[1]][0], None]
        Ye+=[2*M-position[edge[0]][1],2*M-position[edge[1]][1], None]

    G.vs["label"] = [nodes_[i] for i in range(len(tree))]
    visual_style = {}

    visual_style["vertex_size"] = 50
    visual_style["vertex_color"] = ["orange" if nodes_[i] in list(eldest_childs.values()) else "white" for i in range(len(tree)) ]
    visual_style["vertex_label"] = ["PID: " + str(nodes_[i])+"\n"+time.ctime(start_times[nodes_[i]]/1e9) for i in range(len(tree))]
    visual_style["layout"] = lay
    visual_style["bbox"] = (2000, 2000)
    visual_style["margin"] = 200
    os.mkdir("../process_trees")
    igraph.plot(G, "../process_trees/"+sys.argv[1], **visual_style)

main()