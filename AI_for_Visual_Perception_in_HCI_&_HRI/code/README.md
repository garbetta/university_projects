DIRECTORY STRUCTURE

mtcnn = directory that contains the mtcnn model and weights
Final_Application.ipynb = application that can be executed by the user 
HEGClass_TRAINING_file.ipynb = file with all the training steps for the Head-Gaze position classifier 
YOLO_v8_traffic_signs_TRAINING_file.ipynb = file with all the training steps for the YOLOv8 traffic signs classifier 
UTILS.ipynb = function used to preprocess or to 
TRIAL1_HeadGazePose_RESNET+SVM.ipynb = file with the first trial (SVM classifier) not used in the final application, but mentioned in the paper 
TRIAL2_HeadGazePose_RESNET+ClassNet.ipynb =  file with the second trial (ClassNet classifier) not used in the final application, but mentioned in the paper 
TRIAL3_HeadGazePose_VGG16Net(images+rpy+pupils).ipynb =  file with the third trial (VGG16_based classifier) not used in the final application, but mentioned in the paper 
yolo_8_3_classes_best.pt = weights of the trained YOLOv8 model classifier, used in the Final_Application
weights_vgg_pretrained_10classes_1002img(5epochs,32batch).pt = weights of the trained HEGClass model classifier, used in the Final_Application
images_features_for_HEGClass_training.csv = csv file containing all the features extracted by the images and used to train the HEGClass method

RUNNING STEPS to try Final_Application.ipynb

Prepare in the filesystem the directories to save your intermediate and final files of our attention driver’s system: 

FINAL_TEST
	|一 ANNOTATIONS
	|一 FRAMES
	|	|一 EXTERNAL
	|	ட INTERNAL
	ட YOLO_Gaze_detection


To execute the code you have also to prepare the variables:
ROOT = name of the global path containing all the other directories. 
GENERAL_DIR = ROOT + FINAL_TEST, that is the father directory’s path where you can save your external and internal frames (/FRAMES/EXTERNAL or /FRAMES/EXTERNAL) and the final prediction files (/YOLO_Gaze_detection). 


Then, in the EXECUTION PART:

Choose the clips that you want to analyze (one internal, one external). The dimension of the videos isn’t important, but if your videos are longer than 30 seconds and you want to split it, we have prepared a function in UTILS.ipynb.

You have to save the paths of your clips in source_clip_ext, source_clip_in, dest_dir_ext, dest_dir_in and generate the frames automatically with the getFrames_30clip function. 

Initialize the YOLO model and the HEGClass model with our weights (you can find them in the code directory). 

Finally choose the name of your csv final file (default annotations.csv) where the application will save: the analyzed frames’ number, the number of signs present in each external frame, their positions cells, the predicted gaze cell and a boolean value if you are seeing at least one sign.  

Run the loop to make the prediction. For all external frames in which the Net found traffic signs, it analyzes the correspective internal frame and the gaze position. 
If you want to save external images, you can put a desidered path as last parameter of function image_test_cell instead of blank string; for the internal frame use the parameter tot in function extract_features_from_path. 

Finally you can observe the produced annotations.csv file and evaluate your driving attention. 





