SHELL=/bin/bash -euo pipefail

CTXDIR=../..
CTX=$(CTXDIR)/bin/mccortex31
CTXPIPELINE=$(CTXDIR)/scripts/make-pipeline.pl
READSIM=$(CTXDIR)/libs/readsim/readsim
DNACAT=$(CTXDIR)/libs/seq_file/bin/dnacat
SIMMUT=$(CTXDIR)/libs/bioinf-perl/sim_mutations/sim_mutations.pl
CONTIG_STATS=$(CTXDIR)/libs/bioinf-perl/fastn_scripts/contig_stats.pl
BWA=$(CTXDIR)/libs/bwa/bwa
CONTIG_CMP_SCRIPT=$(CTXDIR)/scripts/python/break-contigs-vs-truth.py

K=31
HAPDEPTH=30
READLEN=150
ERRRATE=0.01
MEM=1G
OUTDIR=proj
REF=../data/chr22/chr22_17M_18M.fa

MASKS=diploid/mask0.fa diploid/mask1.fa
CHROMS_ALN=diploid/genome0.fa diploid/genome1.fa
CHROMS=diploid/chrom0.fa diploid/chrom1.fa
READS=reads/chrom0.$(HAPDEPTH)X.1.fa.gz reads/chrom0.$(HAPDEPTH)X.2.fa.gz \
		  reads/chrom0.$(HAPDEPTH)X.B.1.fa.gz reads/chrom0.$(HAPDEPTH)X.B.2.fa.gz \
			reads/chrom1.$(HAPDEPTH)X.1.fa.gz reads/chrom1.$(HAPDEPTH)X.2.fa.gz
DIRS=reads diploid

# Mark all dependencies as secondary
# It means don't re-run if the dependency file disappears -- allows us to delete unused files
.SECONDARY:
# Delete files if their recipe fails
.DELETE_ON_ERROR:
# Remove implicit rules for certain suffixes
.SUFFIXES:

all: run-mccortex haploid-mccortex

# assess:
# 	$(BWA) index $(REF)
# 	$(BWA) mem $(REF) proj/k31/contigs/MissSample.pop.rmdup.fa.gz > tmp.sam

# 1 Mb human diploid
# Generate a diploid genome from a haploid reference
diploid/mask0.fa: diploid/genome0.fa
diploid/mask1.fa: diploid/genome1.fa
diploid/genome0.fa: diploid/genome1.fa
diploid/genome1.fa: $(REF) | $(DIRS)
	$(SIMMUT) --snps 1000 --indels 100 --invs 0 diploid 2 $<

$(CHROMS): $(CHROMS_ALN)

# Remove deletion marks (-) and convert to uppercase
diploid/chrom%.fa: diploid/genome%.fa
	cat $< | tr -d '-' | $(DNACAT) -u -F - > $@

# Simulate PE reads of each chrom each 50X
reads/chrom0.$(HAPDEPTH)X.B.1.fa.gz: reads/chrom0.$(HAPDEPTH)X.B.2.fa.gz
reads/chrom0.$(HAPDEPTH)X.1.fa.gz: reads/chrom0.$(HAPDEPTH)X.2.fa.gz
reads/chrom1.$(HAPDEPTH)X.1.fa.gz: reads/chrom1.$(HAPDEPTH)X.2.fa.gz

reads/chrom%.$(HAPDEPTH)X.2.fa.gz: diploid/chrom%.fa
	$(READSIM) -l $(READLEN) -r $< -d $(HAPDEPTH) -e $(ERRRATE) reads/chrom$*.$(HAPDEPTH)X

reads/chrom%.$(HAPDEPTH)X.B.2.fa.gz: diploid/chrom%.fa
	$(READSIM) -l $(READLEN) -r $< -d $(HAPDEPTH) -e $(ERRRATE) reads/chrom$*.$(HAPDEPTH)X.B

samples.txt:
	echo "MissSample . "\
"reads/chrom0.$(HAPDEPTH)X.1.fa.gz:reads/chrom0.$(HAPDEPTH)X.2.fa.gz,"\
"reads/chrom1.$(HAPDEPTH)X.1.fa.gz:reads/chrom1.$(HAPDEPTH)X.2.fa.gz" > $@

haploid_samples.txt:
	echo "MissSample . "\
"reads/chrom0.$(HAPDEPTH)X.1.fa.gz:reads/chrom0.$(HAPDEPTH)X.2.fa.gz,"\
"reads/chrom0.$(HAPDEPTH)X.B.1.fa.gz:reads/chrom0.$(HAPDEPTH)X.B.2.fa.gz" > $@

task.k$(K).mk: samples.txt
	$(CTXPIPELINE) $(K) $(OUTDIR) $< > $@

haploid.k$(K).mk: haploid_samples.txt
	$(CTXPIPELINE) $(K) happroj $< > $@

unitigs.k$(K).fa: task.k$(K).mk $(READS)
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G graphs
	$(CTX) unitigs -o $@ proj/k31/graphs/MissSample.clean.ctx >& $@.log

run-mccortex: task.k$(K).mk unitigs.k$(K).fa $(READS)
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G POP_BUBBLES_ARGS="--max-len 41" contigs contigs-pop
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G POP_BUBBLES_ARGS="--max-len 41" USE_LINKS=NO contigs contigs-pop
	$(CONTIG_STATS) --genome 1M unitigs.k$(K).fa
	gzip -cd  proj/k31/contigs_plain/MissSample.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  proj/k31/contigs_plain/MissSample.pop.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  proj/k31/contigs_links/MissSample.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  proj/k31/contigs_links/MissSample.pop.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	dnacat -P diploid/chrom0.fa unitigs.k31.fa | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.unitigs.txt
	dnacat -P diploid/chrom0.fa proj/k31/contigs_plain/MissSample.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.plain.txt
	dnacat -P diploid/chrom0.fa proj/k31/contigs_plain/MissSample.pop.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.plain.pop.txt
	dnacat -P diploid/chrom0.fa proj/k31/contigs_links/MissSample.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.links.txt
	dnacat -P diploid/chrom0.fa proj/k31/contigs_links/MissSample.pop.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.links.pop.txt
	@echo "-- stats.unitigs.txt" && cat stats.unitigs.txt
	@echo "-- stats.plain.txt" && cat stats.plain.txt
	@echo "-- stats.plain.pop.txt" && cat stats.plain.pop.txt
	@echo "-- stats.links.txt" && cat stats.links.txt
	@echo "-- stats.links.pop.txt" && cat stats.links.pop.txt

haploid-mccortex: haploid.k$(K).mk $(READS)
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G contigs
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G USE_LINKS=no unitigs contigs
	gzip -cd  happroj/k31/graphs/MissSample.clean.unitigs.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  happroj/k31/contigs_plain/MissSample.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  happroj/k31/contigs_links/MissSample.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	dnacat -P diploid/chrom0.fa happroj/k31/graphs/MissSample.clean.unitigs.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.unitigs.hap.txt
	dnacat -P diploid/chrom0.fa happroj/k31/contigs_plain/MissSample.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.plain.hap.txt
	dnacat -P diploid/chrom0.fa happroj/k31/contigs_links/MissSample.rmdup.fa.gz | $(CONTIG_CMP_SCRIPT) 31 1> /dev/null 2> stats.links.hap.txt
	@echo "-- stats.unitigs.hap.txt" && cat stats.unitigs.hap.txt
	@echo "-- stats.plain.hap.txt" && cat stats.plain.hap.txt
	@echo "-- stats.links.hap.txt" && cat stats.links.hap.txt


$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS) $(OUTDIR) samples.txt task.k$(K).mk unitigs.k$(K).fa*
	rm -rf happroj haploid.k$(K).mk haploid_samples.txt
	rm -rf stats.*.txt

.PHONY: all clean run-mccortex
