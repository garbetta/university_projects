# VisiopeProject

## Vision and Perception Project 2023/2024

In recent years, waste levels have been increasing rapidly, and the only solution is an adequate level of recycling, a mechanism that is not yet fully carried out autonomously. This led us to our project idea: implementing a model that performs waste detection and classification, categorizing it based on the construction material, which can then be used during the sorting of recyclable materials. Our primary goal is to recognize and differentiate objects in outdoor environments (fields, forests, sea, sand, etc.), making the choice of datasets crucial.
Other ideas include working on objects composed of multiple materials to recognize individual components and dealing with images of small-sized waste. We apply super-resolution techniques to identify and classify them.

***How to use the code:***
- To find and classify waste in new images or entire datasets, you can use the code in the *WasteWatch_detection_classification.ipynb* file.
- Change the paths in the following sections: "INFERENCE ON ONE 'NEW' TEST IMAGE" for a single image, "PERFORM INFERENCE ON TEST-SET CSV" for a dataset in CSV format, or "PERFORM INFERENCE ON TEST-LOADER" for a dataset already divided into batches by a DataLoader.
- Use the weights available in the following drive link: [weights](https://drive.google.com/drive/folders/113FMTqUAFaK7DuJb8fpVh_b09HcpbF_C?usp=drive_link) (modify in the "LOAD_MODELS" section).
