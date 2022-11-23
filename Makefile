all:
	nvcc app.cu -I /usr/local/cuda/include -lm -o color_g
test:
	nvcc a.cu -I /usr/local/cuda/include -lm -o a
convert:
	g++ support/convert.cpp -o convert

clean:
	rm color_g

clean1:
	rm app.cu 

clean2:
	rm a.cu, a

cleanc:
	rm convert
