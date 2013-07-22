import argparse
from collections import defaultdict
import difflib
import logging
import os
import sys
import itertools

from Bio import AlignIO, SeqIO, Phylo
from Bio.Align import AlignInfo, MultipleSeqAlignment
from Bio.Align.Applications import ClustalwCommandline
from Bio.SeqUtils.CheckSum import seguid
from matplotlib import pyplot
import pylab
from scipy.cluster.hierarchy import linkage, dendrogram, fcluster
import numpy as np


def remove_duplicates(fasta):
    check_sums = set()
    for record in fasta:
        check_sum = seguid(record.seq)
        if check_sum in check_sums:
            # print ("Ignoring record {0}".format(record.id))
            continue
        check_sums.add(check_sum)
        yield SeqIO.SeqRecord(id="ID=" + record.id,
                              description=record.description,
                              seq=record.seq, name=record.name)


def distance(record1, record2):
    return 1 - difflib.SequenceMatcher(None, str(record1.seq),
                                       str(record2.seq)).ratio()


def main(args):
    logging.basicConfig(filename='example.log', level=logging.DEBUG)

    parser = argparse.ArgumentParser(description="Sequence analysis after "
                                                 "processing")
    parser.add_argument("-i", metavar="fasta_file",
                        help="path to fasta file")
    parser.add_argument("-o", metavar="out_dir",
                        help="directory for results files")

    args = parser.parse_args(args)
    raw_fasta_path = args.i
    out_dir_path = args.o

    if not os.path.exists(out_dir_path):
        os.makedirs(out_dir_path)

    deduplicated_fasta = remove_duplicates(
        SeqIO.parse(raw_fasta_path, "fasta"))
    base = os.path.basename(raw_fasta_path)
    fasta_path = os.path.join(out_dir_path, base)
    SeqIO.write(deduplicated_fasta, fasta_path, "fasta")

    # Multiple sequence alignment
    cline = ClustalwCommandline("clustalw2", infile=fasta_path)
    stdout, stderr = cline()
    logging.info(cline)

    clustalw_result_path = fasta_path.replace(".fasta", ".aln")

    alignment_dict = SeqIO.to_dict(
        AlignIO.read(clustalw_result_path, "clustal"))

    # writing alignment table in .txt
    with open(os.path.join(out_dir_path, "alignment.txt"), "w") as fout:
        fout.write(
            "\n".join(
                [str(record.seq) for record in alignment_dict.itervalues()]))

    # alignment tree drawing
    tree_path = fasta_path.replace(".fasta", ".dnd")
    tree = Phylo.read(tree_path, "newick")
    tree.ladderize()

    # with labels
    Phylo.draw_graphviz(tree, label_func=lambda x: x.name.replace("ID=", ""))
    pylab.savefig(os.path.join(out_dir_path,
                               "figure_with_labels.pdf"))  # need pygraphviz, pylab

    # Clustering
    ids = dict(enumerate(alignment_dict.keys()))
    distance_matrix = np.zeros([len(ids)] * 2)
    for i, j in itertools.combinations(xrange(len(ids)), r=2):
        distance_matrix[i][j] = distance_matrix[j][i] = \
            distance(alignment_dict[ids[i]], alignment_dict[ids[j]])

    # Compute and plot dendrogram
    fig = pylab.figure()
    axdendro = fig.add_axes([0.09, 0.1, 0.2, 0.8])
    Y = linkage(distance_matrix, method="centroid")
    cutoff = 0.5 * max(Y[:, 2])
    clusters = fcluster(Y, cutoff, "distance")
    Z = dendrogram(Y, orientation="right", color_threshold=cutoff)
    axdendro.set_yticks([])

    # Plot distance matrix
    axmatrix = fig.add_axes([0.3, 0.1, 0.6, 0.8])
    index = Z["leaves"]
    distance_matrix = distance_matrix[index, :]
    distance_matrix = distance_matrix[:, index]
    im = axmatrix.matshow(distance_matrix, aspect="auto", origin="lower")
    axmatrix.set_xticks([])
    axmatrix.set_yticks([])

    # Plot colorbar
    axcolor = fig.add_axes([0.91, 0.1, 0.02, 0.8])
    pylab.colorbar(im, cax=axcolor)

    # Display and save figure
    dendogram_path = os.path.join(out_dir_path, "dendogram.png")
    fig.savefig(dendogram_path)

    fasta_clusters = defaultdict(list)
    for i, cluster in enumerate(clusters):
        fasta_id = ids[i]
        fasta_clusters[cluster].append(alignment_dict[fasta_id])

    # Saving information about clusters
    clusters_dir_path = os.path.join(out_dir_path, "clusters")
    if not os.path.exists(clusters_dir_path):
        os.makedirs(clusters_dir_path)
    clusters_meta_path = os.path.join(clusters_dir_path, "clusters_meta.txt")
    meta_file = open(clusters_meta_path, "w")
    for cluster_id, cluster in fasta_clusters.iteritems():
        cluster_path = os.path.join(clusters_dir_path,
                                    "cluster_{0}.fasta".format(cluster_id))
        SeqIO.write(cluster, cluster_path, "fasta")
        summary_align = AlignInfo.SummaryInfo(MultipleSeqAlignment(cluster))
        consensus = summary_align.dumb_consensus()
        pssm = summary_align.pos_specific_score_matrix(consensus,
                                                       chars_to_ignore=['X'])
        frequencies = dict((key, len(list(group))) for key, group in
                           itertools.groupby(consensus))

        meta_file.write("""Cluster ID: {0}
Cluster size: {1}
Consensus: {2}
PSSM:
{3}
Frequencies in consensus: {4}

""".format(cluster_id, len(cluster), consensus, pssm, frequencies))


if __name__ == "__main__":
    main(sys.argv[1:])