SHELL=/bin/bash -euo pipefail

CTXDIR=../..
CTX=$(CTXDIR)/bin/ctx31
CTXPIPELINE=$(CTXDIR)/scripts/make-pipeline.pl
READSIM=$(CTXDIR)/libs/readsim/readsim
DNACAT=$(CTXDIR)/libs/seq_file/bin/dnacat
SIMMUT=$(CTXDIR)/libs/bioinf-perl/sim_mutations/sim_mutations.pl
SIMTRUTH=$(CTXDIR)/libs/bioinf-perl/sim_mutations/sim_vcf.pl
BCFTOOLS=$(CTXDIR)/libs/bcftools/bcftools
BGZIP=$(CTXDIR)/libs/htslib/bgzip
VCFALLELES=$(CTXDIR)/scripts/bash/vcf-count-alleles
CONTIG_STATS=$(CTXDIR)/libs/bioinf-perl/fastn_scripts/contig_stats.pl

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
			reads/chrom1.$(HAPDEPTH)X.1.fa.gz reads/chrom1.$(HAPDEPTH)X.2.fa.gz
DIRS=reads diploid

# Mark all dependencies as secondary
# It means don't re-run if the dependency file disappears -- allows us to delete unused files
.SECONDARY:
# Delete files if their recipe fails
.DELETE_ON_ERROR:
# Remove implicit rules for certain suffixes
.SUFFIXES:

all: run-mccortex

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
reads/chrom0.$(HAPDEPTH)X.1.fa.gz: reads/chrom0.$(HAPDEPTH)X.2.fa.gz
reads/chrom1.$(HAPDEPTH)X.1.fa.gz: reads/chrom1.$(HAPDEPTH)X.2.fa.gz

reads/chrom%.$(HAPDEPTH)X.2.fa.gz: diploid/chrom%.fa
	$(READSIM) -l $(READLEN) -r $< -d $(HAPDEPTH) -e $(ERRRATE) reads/chrom$*.$(HAPDEPTH)X

samples.txt:
	echo "MissSample . "\
"reads/chrom0.$(HAPDEPTH)X.1.fa.gz:reads/chrom0.$(HAPDEPTH)X.2.fa.gz,"\
"reads/chrom1.$(HAPDEPTH)X.1.fa.gz:reads/chrom1.$(HAPDEPTH)X.2.fa.gz" > $@

task.k$(K).mk: samples.txt
	$(CTXPIPELINE) -r $(REF) $(K) $(OUTDIR) $< > $@

run-mccortex: task.k$(K).mk $(READS)
	$(MAKE) -f $< CTXDIR=$(CTXDIR) MEM=1G contigs contigs-pop
	gzip -cd  proj/k31/contigs/MissSample.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -
	gzip -cd  proj/k31/contigs/MissSample.pop.rmdup.fa.gz | $(CONTIG_STATS) --genome 1M -

$(DIRS):
	mkdir -p $@

clean:
	rm -rf $(DIRS) $(OUTDIR) samples.txt task.k$(K).mk truth.*vcf*

.PHONY: all clean run-mccortex