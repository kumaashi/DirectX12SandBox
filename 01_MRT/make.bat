del Win.exe
cl Win.cpp main.cpp  /Ox /GS- /EHsc /I.\include\
del *.obj
Win.exe

