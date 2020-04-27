basisu @basis_file_list.txt -unpack -no_ktx -etc1_only -output_path ./png
copy /B /Y *etc1*.png etc1
del /Q *.png