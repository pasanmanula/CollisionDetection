#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#define A -1
#define B -2
#define C -3
#define SUCCESS 1 //Flags
#define FAILURE 2
#define STOP 3

struct Vertex {
	float xCoor, yCoor;

	Vertex(const Vertex& v)
		: xCoor(v.xCoor), yCoor(v.yCoor)
	{}
	Vertex(float x, float y)
		: xCoor(x), yCoor(y)
	{}
};

std::ostream& operator<<(std::ostream& stream, const Vertex& vertex)
{
	stream << vertex.xCoor << "," << vertex.yCoor;
	return stream;
}

void loadVerticesTo(std::vector<Vertex>& shapeA, std::vector<Vertex>& shapeB, const std::string& shAFile, const std::string& shBFile) {
	std::vector<std::string> temp;
	std::string::size_type sz;
	std::string inputVertices;
	std::ifstream shapeAVertices, shapeBVertices;
	shapeAVertices.open(shAFile);
	shapeBVertices.open(shBFile);
	if ((!shapeAVertices) || (!shapeBVertices)) {
		std::cout << "Unable to open shape files";
		exit(1); // terminate with error
	}
	else {
		//Load shA to memory
		while (shapeAVertices >> inputVertices) {
			//std::cout << inputVertices << std::endl;
			std::stringstream s_stream(inputVertices); //create string stream from the string
			std::string substr;
			while (s_stream.good()) {
				getline(s_stream, substr, ','); //get first string delimited by comma
				temp.emplace_back(substr);
			}
			shapeA.emplace_back(std::stof(temp[0], &sz), std::stof(temp[1], &sz));
			temp.clear();
		}
		//Load shB to memory
		while (shapeBVertices >> inputVertices) {
			//std::cout << inputVertices << std::endl;
			std::stringstream s_stream(inputVertices); //create string stream from the string
			std::string substr;
			while (s_stream.good()) {
				getline(s_stream, substr, ','); //get first string delimited by comma
				temp.emplace_back(substr);
			}
			shapeB.emplace_back(std::stof(temp[0], &sz), std::stof(temp[1], &sz));
			temp.clear();
		}

		shapeAVertices.close();
		shapeBVertices.close();
	}
}

void minkowskiDiff(const Vertex& v1, const Vertex& v2, std::vector<Vertex>& simplex)
{
	Vertex minkowskiDiff = Vertex((v1.xCoor - v2.xCoor), (v1.yCoor - v2.yCoor));
	simplex.emplace_back(minkowskiDiff);
}

int findMaxDotProduct(std::vector<Vertex>& shape, const Vertex& unitVectorD, std::vector<float>& dotProducts, std::vector<float>::iterator& maxPosition)
{
	dotProducts.clear();
	for (Vertex& vertex : shape) {
		dotProducts.emplace_back((vertex.xCoor * unitVectorD.xCoor) + (vertex.yCoor * unitVectorD.yCoor));
	}
	maxPosition = std::max_element(dotProducts.begin(), dotProducts.end());
	return static_cast<int> (std::distance(dotProducts.begin(), maxPosition)); //never planning to have more than 2^32-1 vertices

}
void supportPoint(std::vector<Vertex>& shapeA, std::vector<Vertex>& shapeB, Vertex& unitVectorD, int& supportA, int& supportB)
{
	std::vector<float> dotProducts;
	std::vector<float>::iterator maxPosition;
	Vertex unitVectorDOpposite = Vertex(-1 * unitVectorD.xCoor, -1 * unitVectorD.yCoor); //Opposite vector

	supportA = findMaxDotProduct(shapeA, unitVectorD, dotProducts, maxPosition);
	supportB = findMaxDotProduct(shapeB, unitVectorDOpposite, dotProducts, maxPosition);

}

bool sanityCheck(const std::vector<Vertex>& simplex, const Vertex& unitVectorD)
{
	float dotProduct = (unitVectorD.xCoor * simplex.back().xCoor) + (unitVectorD.yCoor * simplex.back().yCoor);
	if (dotProduct > 0)
	{
		for (std::vector<Vertex>::const_iterator it = simplex.begin(); it != (simplex.end() - 1); it++)
		{
			if ((it->xCoor == simplex.back().xCoor) && (it->yCoor == simplex.back().yCoor))
			{
				std::cout << "Shapes do not intersect" << std::endl;
				return false;
			}
		}
		return true;
	}
	else {
		return false;
	}
}

Vertex tripleCrossProductTriangle(const Vertex& a, const Vertex& b, const Vertex& c) //(A X B) X C = (a.c)b - (c.b)a
{
	//Assuming only for 2D shapes.
	float dotOne = (a.xCoor * c.xCoor) + (a.yCoor * c.yCoor);
	float dotTwo = (c.xCoor * b.xCoor) + (c.yCoor * b.yCoor);
	Vertex newDir = Vertex(((dotOne * b.xCoor) - (dotTwo * a.xCoor)), ((dotOne * b.yCoor) - (dotTwo * a.yCoor)));
	return newDir;
}

//Use this function if the simplex already contains more than 2 vertices.
int triangleCase(std::vector<Vertex>& shapeA, std::vector<Vertex>& shapeB, std::vector<Vertex>& simplex, Vertex& unitVectorD, const Vertex& origin, int& posA, int& posB) {
	//Finding the new point A
	supportPoint(shapeA, shapeB, unitVectorD, posA, posB);
	minkowskiDiff(shapeA[posA], shapeB[posB], simplex); //add the new point to the last. Denoted as A
	if (sanityCheck(simplex, unitVectorD)) {
		//Check region AB
		//Checking if the newly formed triangle contains the origin.
		Vertex ac = Vertex((simplex.end()[C].xCoor - simplex.end()[A].xCoor), (simplex.end()[C].yCoor - simplex.end()[A].yCoor));
		Vertex ab = Vertex((simplex.end()[B].xCoor - simplex.end()[A].xCoor), (simplex.end()[B].yCoor - simplex.end()[A].yCoor));
		Vertex ao = Vertex((origin.xCoor - simplex.end()[A].xCoor), (origin.yCoor - simplex.end()[A].yCoor));
		unitVectorD = tripleCrossProductTriangle(ac, ab, ab);
		float dotResult = (unitVectorD.xCoor * ao.xCoor) + (unitVectorD.yCoor * ao.yCoor);
		if (dotResult > 0) {
			simplex.erase(simplex.begin());
			return FAILURE;
		}
		else
		{
			//Check region AC
			unitVectorD = tripleCrossProductTriangle(ab, ac, ac);
			float dotResult = (unitVectorD.xCoor * ao.xCoor) + (unitVectorD.yCoor * ao.yCoor);
			if (dotResult > 0)
			{
				simplex.erase(simplex.begin());
				return FAILURE;
			}
			else {
				std::cout << "Shapes do intersect! " << dotResult << std::endl;
				return SUCCESS;
			}
		}
	}
	else
	{
		std::cout << "Shapes do not intersect!" << std::endl;
		return STOP;
	}
}


int main()
{

	std::vector<Vertex> shapeA;
	std::vector<Vertex> shapeB;
	std::vector<Vertex> simplex;
	simplex.reserve(3);

	int posA, posB;
	int continueAddingPoints = FAILURE;
	Vertex unitVectorD = Vertex(1.0f, 0.0f); //Starting Direction
	Vertex origin = Vertex(0.0f, 0.0f);

	std::string shAVertices;
	std::string shBVertices;
	std::cout << "Shape A file : ";
	std::cin >> shAVertices;
	std::cout << "Shape B file : ";
	std::cin >> shBVertices;

	try
	{
		loadVerticesTo(shapeA, shapeB, shAVertices, shBVertices); //Read from txt file and load it to memory.
		throw 20;
	}
	catch (int e)
	{
		std::cout << "An exception occurred. " << e << '\n';
		return 0;
	}

	//First Point
	supportPoint(shapeA, shapeB, unitVectorD, posA, posB); //Returns through POSA,POSB. Furthest points
	minkowskiDiff(shapeA[posA], shapeB[posB], simplex); //Add the point to the simplex

	//Second Point	
	//Next towards the origin. Get support points, calc Minkowski and add to the simplex if sanity check passes.
	unitVectorD = Vertex((origin.xCoor - simplex.back().xCoor), (origin.yCoor - simplex.back().yCoor));
	supportPoint(shapeA, shapeB, unitVectorD, posA, posB);
	minkowskiDiff(shapeA[posA], shapeB[posB], simplex);
	if (!sanityCheck(simplex, unitVectorD)) {
		std::cout << "Shapes do not intersect!" << std::endl;
		return 0;
	}

	//Third Point
	//Next select normal vector to the current line segment facing the origin. Get support points, calc Minkowski and add to the simplex if sanity check passes.
	Vertex bc = Vertex((simplex.front().xCoor - simplex.back().xCoor), (simplex.front().yCoor - simplex.back().yCoor));
	Vertex bo = Vertex((origin.xCoor - simplex.back().xCoor), (origin.yCoor - simplex.back().yCoor));
	unitVectorD = tripleCrossProductTriangle(bc, bo, bc);

	while (continueAddingPoints == FAILURE) {
		continueAddingPoints = triangleCase(shapeA, shapeB, simplex, unitVectorD, origin, posA, posB);
	}
}


