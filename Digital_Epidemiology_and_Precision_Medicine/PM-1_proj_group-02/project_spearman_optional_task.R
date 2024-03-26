library(dplyr)
library(psych)
library(ggplot2);library(ggnet);
library(igraph);library(GGally);
library(sna);library(network);library(gplots)
library('biomaRt')
library(stringr)

library(DAAG) 

library(DESeq2)
library(RColorBrewer)
library(pheatmap)
library(tidyverse)
library(data.table)

library(TCGAbiolinks)
library(SummarizedExperiment)
library(DT)
library(readr)
library(ggrepel)

library(lsa)
library(reshape2)
library(corrplot)




###############################################################################
################################ DOWNLOAD DATA ################################

path_proj <- "C:/Users/aless/Desktop/R-DEPM/Project-1/TCGA-COAD"
path_data <- "C:/Users/aless/Desktop/R-DEPM/Project-1/GDCdata"

proj <- "TCGA-COAD" #colon adenocarcinoma

dir.create(file.path(path_proj))

#preparazione della query da fare per il download

C.query.exp <- GDCquery(project = proj, data.category = "Transcriptome Profiling",
                     data.type = "Gene Expression Quantification",
                     workflow.type = "STAR - Counts", 
                     sample.type = "Primary Tumor")

#download of the data --> The data from query will be save in a folder: project/data.category
#GDCdownload(query = C.query.exp, directory = path_data, method = "api")


N.query.exp <- GDCquery(project = proj, data.category = "Transcriptome Profiling",
                          data.type = "Gene Expression Quantification",
                          workflow.type = "STAR - Counts", 
                          sample.type = "Solid Tissue Normal")

#GDCdownload(query = N.query.exp, directory = path_data, method = "api")

#prepare transforms the downloaded data into a summarizedExperiment
C.data.exp <- GDCprepare(C.query.exp, directory = path_data)

N.data.exp <- GDCprepare(N.query.exp, directory = path_data)

C.expr.data <- assay(C.data.exp)
norm.data.C <- C.data.exp@assays@data@listData[["fpkm_unstrand"]]
rownames(norm.data.C) <- rownames(C.expr.data)
colnames(norm.data.C) <- colnames(C.expr.data)
#write.csv(norm.data.C, "C:/Users/aless/Desktop/R-DEPM/Project-1/rna_expr_data_C.csv",sep = ",", row.names=TRUE, col.names=TRUE, quote = FALSE)


N.expr.data <- assay(N.data.exp)
norm.data.N <- N.data.exp@assays@data@listData[["fpkm_unstrand"]]
rownames(norm.data.N) <- rownames(N.expr.data)
colnames(norm.data.N) <- colnames(N.expr.data)
#write.csv(norm.data.N, "C:/Users/aless/Desktop/R-DEPM/Project-1/rna_expr_data_N.csv",sep = ",", row.names=TRUE, col.names=TRUE, quote = FALSE)


clinical.query <- GDCquery_clinic(project = "TCGA-COAD", type = "clinical", save.csv = FALSE)
#write.csv(clinical.query, file = "C:/Users/aless/Desktop/R-DEPM/Project-1/prog_clinical_data.txt", row.names = FALSE, quote = FALSE)

###############################################################################
############################# PRE-PROCESSING DATA #############################

length(rownames(clinical.query)) #461
length(colnames(norm.data.C)) #481
length(colnames(norm.data.N)) #41

unique(substr(colnames(norm.data.N), 1,12)) #41         #1, 12 sono i caratteri che prende del nome delle colonne(prende i primi 12 caratteri)
unique(substr(colnames(norm.data.C), 1,12)) #456   ->   more samples for some patients

patients <- substr(colnames(norm.data.C), 1,12) #who has more samples?
(table(patients))
table(patients)[order(table(patients))]

unique.patients <- names(which(table(patients) == 1))
#retrieving their index in the column list
unique.patients.idx <- match(unique.patients, substr(colnames(norm.data.C), 1,12) )

#let's consider only patients for which I have one sample
expr.C <- as.data.frame(norm.data.C)
expr.C <- expr.C[,unique.patients.idx]

expr.N <- as.data.frame(norm.data.N)

#renaming the patients  in the exrpession matrices
colnames(expr.C) <- substr(colnames(expr.C), 1,12)
unique(colnames(expr.C)) #443 

colnames(expr.N) <- substr(colnames(expr.N), 1,12)
unique(colnames(expr.N))

length(intersect(colnames(expr.N), colnames(expr.C)))
#three patients are not common, they need to be removed
setdiff(colnames(expr.N), colnames(expr.C))
#these are the indices of the 3 patients
match(setdiff(colnames(expr.N), colnames(expr.C)), colnames(expr.N)) 
expr.N <- expr.N[,-c(2,5,16)]

intersect(colnames(expr.N), colnames(expr.C)) #38, great
#we will only consider the common patients
expr.C <- expr.C %>% dplyr::select(intersect(colnames(expr.C), colnames(expr.N))) 

#are the values in the dataframe numbers? 
typeof(expr.C[1,1]) 
typeof(expr.N[1,1])
#are there any Nas or Nans?
any(is.na(expr.C))            
any(is.nan(as.matrix(expr.C))) 
any(is.na(expr.N)) 
any(is.nan(as.matrix(expr.N))) 

#as final preprocessing step, let's remove the genes which have zero values
#how many genes have no zeros in the data frame?
sum(rowSums(expr.C == 0) == 0) #15722
no.zeros.genes <- rownames(expr.C)[rowSums(expr.C == 0) == 0] #these are their names

sum(rowSums(expr.N == 0) == 0) #20475
no.zeros.genes2 <- rownames(expr.N)[rowSums(expr.N == 0) == 0] #these are their names

length(intersect(no.zeros.genes, no.zeros.genes2)) #15488
#let's consider only these genes 
filtr.expr.c <- expr.C[intersect(no.zeros.genes, no.zeros.genes2),] 
filtr.expr.n <- expr.N[intersect(no.zeros.genes, no.zeros.genes2),]


#for today's examples let's focus on a smaller set of genes 
all(rownames(filtr.expr.c) == rownames(filtr.expr.n))   #prova che prende in entrabi gli stessi "pazienti"

final.expr.c <- filtr.expr.c    #38 pazienti tot geni
final.expr.n <- filtr.expr.n    #38 pazienti tot geni

final.expr.n <- setcolorder(final.expr.n, c(sort(colnames(final.expr.n))))

nrow(final.expr.c)
#head(final.expr.c)    #stampa le tabelle
#head(final.expr.n)

############################## FINE PRE-PROCESSING ######################################
#########################################################################################


############################ (2) DIFFERENTIALLY EXPRESSED GENES ##########################

x <- final.expr.n
y <- final.expr.c


colnames(y) <- c("TCGA-A6-2671c", "TCGA-A6-2675c", "TCGA-A6-2678c", "TCGA-A6-2679c", "TCGA-A6-2680c", # nolint
                "TCGA-A6-2682c", "TCGA-A6-2683c", "TCGA-A6-2685c", "TCGA-A6-2686c", "TCGA-A6-5662c", # nolint
                "TCGA-A6-5667c", "TCGA-AA-3489c", "TCGA-AA-3496c", "TCGA-AA-3511c", "TCGA-AA-3514c", # nolint
                "TCGA-AA-3516c", "TCGA-AA-3517c", "TCGA-AA-3518c", "TCGA-AA-3520c", "TCGA-AA-3522c", # nolint
                "TCGA-AA-3525c", "TCGA-AA-3527c", "TCGA-AA-3531c", "TCGA-AA-3534c", "TCGA-AA-3655c", # nolint
                "TCGA-AA-3660c", "TCGA-AA-3662c", "TCGA-AA-3663c", "TCGA-AA-3697c", "TCGA-AA-3712c", # nolint
                "TCGA-AA-3713c", "TCGA-AZ-6598c", "TCGA-AZ-6599c", "TCGA-AZ-6600c", "TCGA-AZ-6601c", # nolint
                "TCGA-AZ-6603c", "TCGA-AZ-6605c", "TCGA-F4-6704c")
names(y)
merge_t <- cbind(x, y)
#head(merge_t)
#names(merge_t)

listc <- names(merge_t)
cond <- c("control","control","control","control","control", "control","control","control","control", "control","control", # nolint
        "control","control","control","control","control","control","control","control","control", # nolint
        "control","control","control","control","control","control","control","control","control", # nolint
        "control","control","control","control","control","control","control","control","control", # nolint
        "infected","infected","infected","infected","infected","infected","infected","infected","infected", "infected", "infected", # nolint
        "infected","infected","infected","infected","infected","infected","infected","infected","infected", # nolint
        "infected","infected","infected","infected","infected","infected","infected","infected","infected", # nolint
        "infected","infected","infected","infected","infected","infected","infected","infected","infected") # nolint

coldata <- data.frame(sample = listc, condition = cond, row.names = "sample")
coldata$condition <- as.factor(coldata$condition)
#view(coldata)

merge_mat <- as.matrix(merge_t)

dds <- DESeqDataSetFromMatrix(countData = round(merge_mat), colData = coldata, design = ~ condition)

dds$condition <- relevel(dds$condition, ref = "control")

dds_se <- DESeq(dds)
# add lfcThreshold (default 0) parameter if you want to filter genes based on log2 fold change
res <- results(dds_se, alpha = 0.05, lfcThreshold = 1.2)  #altHypothesis="greaterAbs" parametro per res
summary(res)

#normalized_counts <- counts(dds_se, normalized=TRUE)
#head(normalized_counts)


dis <- plotMA(res, ylim=c(-8,8))
abline(h=c(-2,2),lwd=5)

res[order(res$padj),]  

#view(res[order(res$padj),])

write.csv(as.data.frame(res[order(res$padj),] ), file="C:/Users/aless/Desktop/R-DEPM/Project-1/res_DGE_padj_c_vs n_dge_spearman.csv")

DEG_thr <- ifelse(abs(res[,c("log2FoldChange")]) >= 1.2 & (res[,c("pvalue")]) <= 0.05 , rownames(res), 0)
#DEG_pval <- ifelse((res[,c("pvalue")]) <= 0.05, rownames(res), 0)
#DEG_pval <- intersect(rownames(merge_mat), DEG_pval)
DEG <- intersect(rownames(merge_mat), DEG_thr)  #list of genes with desired FC
#DEG <- intersect(DEG_thr, DEG_pval)


#order(DEG_thr) == order(DEG)

EM <- read.csv(file = "C:/Users/aless/Desktop/R-DEPM/Project-1/res_DGE_padj_c_vs n_dge.csv_spearman", row.names = 1)

df <- data.frame(log10FC = (EM$log2FoldChange),
                 pv = (EM$pvalue))

df$diffexpressed <- "NO"
# if log2Foldchange > 1.2 and pvalue < 0.05, set as "UP" 
df$diffexpressed[df$log10FC > 2 & df$pv < 0.05] <- "UP"
# if log2Foldchange < -1.2 and pvalue < 0.05, set as "DOWN"
df$diffexpressed[df$log10FC < -2 & df$pv < 0.05] <- "DOWN"


df$delabel <- NA
df$delabel[df$diffexpressed != "NO"] <- df$gene_symbol[df$diffexpressed != "NO"]

volcano <- ggplot(data=df, aes(x=log10FC, y=(-log10(pv)), col=diffexpressed, label=delabel)) +
        geom_point() + 
        theme_minimal() +
        geom_text_repel() +
        scale_color_manual(values=c("blue", "black", "red")) +
        geom_vline(xintercept=c(-2, 2), col="red") +
        geom_hline(yintercept=-log10(0.05), col="red")


#############################################################################################
############################# (3) CO-EXPRESSION NETWORKS ####################################

#### ADJ MATRIX for cancer network ###

final.expr.c <- y[DEG, ]
nrow(final.expr.c)
ncol(final.expr.c)

final.expr.c <- log2(final.expr.c+1)
#view(final.expr.c)
#final_c <- join(final.expr.c, DEG)

cor.mat.c <- cor(t(final.expr.c), method = "spearman") #cor --> calcola i coefficienti di correlazione
ncol(cor.mat.c)
nrow(cor.mat.c)
diag(cor.mat.c) <- 0          

cor.padj.c <- corr.p(cor.mat.c, nrow(cor.mat.c), adjust="fdr", ci=FALSE)$p
#view(cor.padj.c)  # non è simmetrica
cor.padj.c[lower.tri(cor.padj.c)] <- t(cor.padj.c)[lower.tri(cor.padj.c)]
#view(cor.padj.c)


adj.mat.c1 <- ifelse(cor.mat.c >= 0.7, 1, ifelse(cor.mat.c <= -0.7, -1, 0))
adj.mat.c2 <- ifelse(abs(cor.padj.c) > 0.05, 0, 1)

adj.mat.c <- adj.mat.c1 * adj.mat.c2
#view(adj.mat.c)

#### ADJ MATRIX for normal network ###

final.expr.n <- x[DEG, ]
nrow(final.expr.n)
ncol(final.expr.n)

final.expr.n <- log2(final.expr.n+1)
#view(final.expr.n)

cor.mat.n <- cor(t(final.expr.n), method = "spearman") #cor --> calcola i coefficienti di correlazione
ncol(cor.mat.n)
nrow(cor.mat.n)
diag(cor.mat.n) <- 0          

cor.padj.n <- corr.p(cor.mat.n, nrow(cor.mat.n), adjust="fdr", ci=FALSE)$p
cor.padj.n[lower.tri(cor.padj.n)] <- t(cor.padj.n)[lower.tri(cor.padj.n)]

adj.mat.n1 <- ifelse(cor.mat.n >= 0.7, 1, ifelse(cor.mat.n <= -0.7, -1, 0 ))
adj.mat.n2 <- ifelse(abs(cor.padj.n) > 0.05, 0, 1)
adj.mat.n <- adj.mat.n1 * adj.mat.n2

########### CO-EXPRESSION CANCER NETWORK ################ 

net.c <- network(adj.mat.c, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")
network.density(net.c) #0.01236725
nrow(component.largest(net.c, result = "graph")) #671


sum(adj.mat.c != 0) #17794
#how many positive/negative correlations? 
sum(adj.mat.c == 1) #17540
sum(adj.mat.c == -1) #254

degree.c <- sort(rowSums(abs(adj.mat.c)), decreasing = TRUE)  #matrice di adiacenza binaria, undirect
sum(degree.c == 0) #455

hist(degree.c)
qua <- quantile(degree.c[degree.c>0], 0.95) #how big is the degree of the most connected nodes? --> 185
hist(degree.c)
abline(v=qua, col="red") #v = valore del quantile

hubs.c <- degree.c[degree.c>qua]
names(hubs.c) #39

net.c %v% "type" = ifelse(network.vertex.names(net.c) %in% names(hubs.c),"hub", "non-hub")
net.c %v% "color" = ifelse(net.c %v% "type" == "hub", "tomato", "deepskyblue3")
set.edge.attribute(net.c, "edgecolor", ifelse(net.c %e% "weights" > 0, "green", "blue"))

coord.c <- gplot.layout.fruchtermanreingold(net.c, NULL)
net.c %v% "x" = coord.c[, 1]
net.c %v% "y" = coord.c[, 2]

ggnet2(net.c, color = "color", alpha = 0.7, size = 2,  mode= c("x","y"),
       edge.color = "edgecolor", edge.alpha = 1, edge.size = 0.15)+
  guides(size = "none") 

#let's get the hubs' names
hubs.c
hubs.c.clean <- (str_split(names(hubs.c), "[.]",simplify = TRUE))[,1] 
mart <- useDataset("hsapiens_gene_ensembl", useMart("ensembl"))
hubs.names <- getBM(filters= "ensembl_gene_id", attributes= c("ensembl_gene_id", "hgnc_symbol"), values=hubs.c.clean, mart= mart)
#ENSG00000000971 --> CFH

#let's order them by degree
hubs.c.ord <- (str_split(names(sort(hubs.c, decreasing = TRUE)), "[.]",simplify = TRUE))[,1]
hubs.names <- hubs.names[match(hubs.c.ord, hubs.names$ensembl_gene_id),]
hubs.names

hubs.names$hgnc_symbol
cat(paste(hubs.names[,2], collapse='\n' ) )


### Sub network cancer hubs ###
hubs.c.ids <- vector("integer", length(hubs.c))
for (i in 1:length(hubs.c)){hubs.c.ids[i] <- match(names(hubs.c)[i],rownames(adj.mat.c))}
hubs.c.ids

#identifying the neighborhood
hubs.c.neigh <- c()
for (f in hubs.c.ids){
  hubs.c.neigh <- append(hubs.c.neigh, get.neighborhood(net.c, f))
}

hubs.c.neigh <- unique(hubs.c.neigh)
hubs.c.neigh
hubs.c.neigh.names <- rownames(adj.mat.c[hubs.c.neigh,])
subnet <- unique(c(names(hubs.c), hubs.c.neigh.names))

#creating the subnetwork
hub.c.adj <- adj.mat.c[subnet, subnet]

rownames(hub.c.adj)[1:length(hubs.c)] <- hubs.names$hgnc_symbol
colnames(hub.c.adj)[1:length(hubs.c)] <- hubs.names$hgnc_symbol
head(rownames(hub.c.adj))
head(colnames(hub.c.adj))

net.hub <- network(hub.c.adj, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")
network.density(net.hub)

sum(hub.c.adj == 1)
sum(hub.c.adj == -1)

net.hub %v% "type" = ifelse(network.vertex.names(net.hub) %in% hubs.names$hgnc_symbol,"hub", "non-hub")
net.hub %v% "color" = ifelse(net.hub %v% "type" == "non-hub", "deepskyblue3", "tomato")
set.edge.attribute(net.hub, "ecolor", ifelse(net.hub %e% "weights" > 0, "green", "blue"))

coord.hubsc <- gplot.layout.fruchtermanreingold(net.hub, NULL)
net.hub %v% "x" = coord.hubsc[, 1]
net.hub %v% "y" = coord.hubsc[, 2]

ggnet2(net.hub,  color = "color",alpha = 0.9, size = 3, mode = c("x", "y"),
       edge.color = "ecolor", edge.alpha = 0.9,  edge.size = 0.15, 
       node.label = hubs.names$hgnc_symbol, label.color = "black", label.size = 4)+
  guides(size = "none")


############## CO-EXPRESSION NORMAL NETWORK ##############

net.n <- network(adj.mat.n, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")
network.density(net.n) #0.03116486
nrow(component.largest(net.n, result = "graph")) #937

sum(adj.mat.n != 0) #44840
#how many positive/negative correlations? 
sum(adj.mat.n == 1) #39146
sum(adj.mat.n == -1) #5694

degree.n <- sort(rowSums(abs(adj.mat.n)), decreasing = TRUE)
sum(degree.n == 0) #234

hist(degree.n)# 
qua <- quantile(degree.n[degree.n>0],0.95) #how big is the degree of the most connected nodes? 445
hist(degree.n)
abline(v=qua, col="red")

hubs.n <- degree.n[degree.n > qua]
names(hubs.n) #49

net.n %v% "type" = ifelse(network.vertex.names(net.n) %in% names(hubs.n),"hub", "non-hub")
net.n %v% "color" = ifelse(net.n %v% "type" == "hub", "tomato", "deepskyblue3")
set.edge.attribute(net.n, "edgecolor", ifelse(net.n %e% "weights" > 0, "green", "blue"))

coord.n <- gplot.layout.fruchtermanreingold(net.n, NULL)
net.n %v% "x" = coord.n[, 1]
net.n %v% "y" = coord.n[, 2]

ggnet2(net.n, color = "color", alpha = 0.65, size = 2, mode = c("x","y"),
       edge.color = "edgecolor", edge.alpha = 1, edge.size = 0.09)+
  guides(size = "none") 

#let's get the hubs' names
hubs.n
hubs.n.clean <- (str_split(names(hubs.n), "[.]",simplify = TRUE))[,1]
mart <- useDataset("hsapiens_gene_ensembl", useMart("ensembl"))
hubs.names2 <- getBM(filters= "ensembl_gene_id", attributes= c("ensembl_gene_id", "hgnc_symbol"), values=hubs.n.clean, mart= mart)

#let's order them by degree
sort(hubs.n, decreasing = TRUE)
hubs.n.ord <- (str_split(names(sort(hubs.n, decreasing = TRUE)), "[.]",simplify = TRUE))[,1]
hubs.names2 <- hubs.names2[match(hubs.n.ord, hubs.names2$ensembl_gene_id),]
hubs.names2

hubs.names2$hgnc_symbol
cat(paste(hubs.names2[,2], collapse='\n' ) )

#intersect(hubs.names$hgnc_symbol, hubs.names2$hgnc_symbol)

only_cancer <- setdiff(hubs.names$hgnc_symbol, hubs.names2$hgnc_symbol) #23
#write(only_cancer, "C:/Users/aless/Desktop/R-DEPM/Project-1/only_cancer_gene.txt", sep = "\n")


### Sub network normal hubs ###

hubs.n.ids <- vector("integer", length(hubs.names2))
for (i in 1:length(hubs.names2)){hubs.n.ids[i] <- match(names(hubs.n)[i],rownames(adj.mat.n))}
hubs.n.ids

#identifying the neighborhood
hubs.n.neigh <- c()
for (f in hubs.n.ids){
  hubs.n.neigh <- append(hubs.n.neigh, get.neighborhood(net.n, f))
}

hubs.n.neigh <- unique(hubs.n.neigh)
hubs.n.neigh
hubs.n.neigh.names <- rownames(adj.mat.n[hubs.n.neigh,])
subnet <- unique(c(names(hubs.n), hubs.n.neigh.names))

#creating the subnetwork
hub.n.adj <- adj.mat.n[subnet, subnet]

rownames(hub.n.adj)[1:length(hubs.names2)] <- hubs.names2$hgnc_symbol
colnames(hub.n.adj)[1:length(hubs.names2)] <- hubs.names2$hgnc_symbol
head(rownames(hub.n.adj))
head(colnames(hub.n.adj))

net.hub <- network(hub.n.adj, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")
network.density(net.hub)

sum(hub.n.adj == 1)
sum(hub.n.adj == -1)

net.hub %v% "type" = ifelse(network.vertex.names(net.hub) %in% hubs.names2$hgnc_symbol,"hub", "non-hub")
net.hub %v% "color" = ifelse(net.hub %v% "type" == "non-hub", "deepskyblue3", "tomato")
set.edge.attribute(net.hub, "ecolor", ifelse(net.hub %e% "weights" > 0, "green", "blue"))

coord.hubsc <- gplot.layout.fruchtermanreingold(net.hub, NULL)
net.hub %v% "x" = coord.hubsc[, 1]
net.hub %v% "y" = coord.hubsc[, 2]

ggnet2(net.hub,  color = "color",alpha = 0.9, size = 3, mode = c("x", "y"),
       edge.color = "ecolor", edge.alpha = 0.9,  edge.size = 0.15, 
       node.label = hubs.names2$hgnc_symbol, label.color = "black", label.size = 4)+
  guides(size = "none")



############################################################################################################
################################ (4) Differential Co-expressed Network ###################################### 

z_diff.c <- fisherz(cor.mat.c)
z_diff.n <- fisherz(cor.mat.n)
nrow(z_diff.c)
view(z_diff.c)

z_score <- (z_diff.c - z_diff.n)/(sqrt((1/(1440000-3)) + (1/(1440000-3))))

view(z_score)


adj.mat.z <- ifelse(abs(z_score) < 0.9,  1, 0)
adj.mat <- adj.mat.z 
diag(adj.mat) <- 0  

sum(adj.mat != 0) #2892

net.z <- network(adj.mat, matrix.type="adjacency", ignore.eval = FALSE, names.eval = "weights")
network.density(net.z) # 0.002010008
nrow(component.largest(net.z, result = "graph")) #1046

degree.z <- sort(rowSums(abs(adj.mat)), decreasing = TRUE)  #matrice di adiacenza binaria, undirect
sum(degree.z == 0) #122


quan <- quantile(degree.z[degree.z>0], 0.95) #how big is the degree of the most connected nodes? --> 185
hist(degree.z)# 
abline(v=quan, col="red") #v = 6

hubs.z <- degree.z[degree.z > quan]
names(hubs.z) # 28

net.z %v% "type" = ifelse(network.vertex.names(net.z) %in% names(hubs.z),"hub", "non-hub")
net.z %v% "color" = ifelse(net.z %v% "type" == "hub", "tomato", "deepskyblue3")
set.edge.attribute(net.z, "edgecolor", ifelse(net.z %e% "weights" > 0, "green", "blue"))

coord.z <- gplot.layout.fruchtermanreingold(net.z, NULL)
net.z %v% "x" = coord.z[, 1]
net.z %v% "y" = coord.z[, 2]

ggnet2(net.z, color = "color", alpha = 0.7, size = 2,  mode= c("x","y"),
       edge.color = "edgecolor", edge.alpha = 1, edge.size = 0.15)+
  guides(size = "none") 

#let's get the hubs' names
hubs.z
hubs.z.clean <- (str_split(names(hubs.z), "[.]",simplify = TRUE))[,1] #nome geni hub, senza valore dopo il punto
mart <- useDataset("hsapiens_gene_ensembl", useMart("ensembl"))
hubs.names.z <- getBM(filters= "ensembl_gene_id", attributes= c("ensembl_gene_id", "hgnc_symbol"), values=hubs.z.clean, mart= mart)
#:ENSG00000000971 --> CFH

#let's order them by degree
#sort(hubs.c, decreasing = TRUE)
hubs.z.ord <- (str_split(names(sort(hubs.z, decreasing = TRUE)), "[.]",simplify = TRUE))[,1]
hubs.names.z <- hubs.names.z[match(hubs.z.ord, hubs.names.z$ensembl_gene_id),]
hubs.names.z

hubs.names.z$hgnc_symbol
cat(paste(hubs.names.z[,2], collapse='\n'))



######################################################################################

############################# (5) PSN - CANCER PATIENTS #############################

final.expr.c <- filtr.expr.c    #38 pazienti tot geni

pat.mat.p <- cor((final.expr.c), method = "spearman")
#view(data.frame(pat.mat.c))
view(pat.mat.p)

corrplot(pat.mat.p,        # Correlation matrix
         method = "shade", # Correlation plot method
         order = 'hclust',
         type = "full",    # Correlation plot style (also "upper" and "lower")
         diag = TRUE,      # If TRUE (default), adds the diagonal
         tl.col = "black", # Labels color
         col.lim = c(0.7, 1),
         #tl.pos = "n",
         bg = "white",     # Background color
         title = "PSN - Cancer",       # Main title
         col = NULL,       # Color palette
         is.corr = TRUE)

min(pat.mat.p)
max(pat.mat.p)

net.p <- network(pat.mat.p, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")
network.density(net.p) #1
nrow(component.largest(net.p, result = "graph")) #38

#NET PEARSON
set.edge.attribute(net.p, "edgecolor", ifelse(net.p %e% "weights" > 0.8, "blue", (ifelse(net.p %e% "weights" < 0.4, "white", "green"))))

coord.p <- gplot.layout.fruchtermanreingold(net.p, NULL)
net.p %v% "x" = coord.c[, 1]
net.p %v% "y" = coord.c[, 2]

ggnet2(net.p, color = "color", alpha = 0.9, size = 20,  mode= c("x","y"),
       edge.color = "edgecolor", edge.alpha = 1, edge.size = 0.15)+
  guides(size = "none") 

################################### LOUVAIN CLUSTERS ##################################################

G2 <- graph.adjacency(pat.mat.p, mode = "undirected", weighted = TRUE, diag = TRUE)
clusterlouvain <- cluster_louvain(G2)
membership(clusterlouvain)
sizes(clusterlouvain)
plot(G2, vertex.color=rainbow(length(clusterlouvain), alpha=0.6)[clusterlouvain$membership])


############################# (5.opt) PSN - NORMAL PATIENTS #############################

final.expr.n <- filtr.expr.n    #38 pazienti tot geni

pat.mat.pn <- cor((final.expr.n), method = "spearman")
view(data.frame(pat.mat.pn))
view(pat.mat.pn)

min(pat.mat.pn)

corrplot(pat.mat.pn,        # Correlation matrix
         method = "shade", # Correlation plot method
         order = 'hclust',
         type = "full",    # Correlation plot style (also "upper" and "lower")
         diag = TRUE,      # If TRUE (default), adds the diagonal
         tl.col = "black", # Labels color
         col.lim = c(0.5, 1),
         #tl.pos = "n",
         bg = "white",     # Background color
         col = NULL,       # Color palette
         is.corr = TRUE)

net.pn <- network(pat.mat.pn, matrix.type="adjacency",ignore.eval = FALSE, names.eval = "weights")

#NET PEARSON
set.edge.attribute(net.pn, "edgecolor", ifelse(net.pn %e% "weights" > 0.8, "blue", (ifelse(net.pn %e% "weights" < 0.4, "white", "green"))))

coord.pn <- gplot.layout.fruchtermanreingold(net.pn, NULL)
net.pn %v% "x" = coord.n[, 1]
net.pn %v% "y" = coord.n[, 2]

ggnet2(net.pn, color = "color", alpha = 0.9, size = 20,  mode= c("x","y"),
       edge.color = "edgecolor", edge.alpha = 1, edge.size = 0.15)+
  guides(size = "none") 

################################### LOUVAIN CLUSTERS ##################################################

G2_n <- graph.adjacency(pat.mat.pn, mode = "undirected", weighted = TRUE, diag = TRUE)
clusterlouvain <- cluster_louvain(G2_n)
membership(clusterlouvain)
sizes(clusterlouvain)
plot(G2_n, vertex.color=rainbow(length(clusterlouvain), alpha=0.6)[clusterlouvain$membership])


