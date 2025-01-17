//
// Created by birdpeople on 2/25/2022.
//

#include "FEMengine.h"

string colorModeNameBuffer[8]={"FORCE_MAGNITUDE",
                               "FORCE_X",
                               "FORCE_Y",
                               "FORCE_Z",
                               "VELOCITY_MAGNITUDE",
                               "VELOCITY_X",
                               "VELOCITY_Y",
                               "VELOCITY_Z"};


inline Eigen::Vector3f FEMengine::colorMap(float num){
    typedef Eigen::Vector3f v3f;
float index = (num > colorLarge ? colorLarge : num) / (this -> colorLarge - this -> colorLess) * 600;
v3f ans = index >= 400 ?
        ((index- 400) * v3f(0,-1.0f/200,0) + v3f(1.0f,1.0f,0)) :
        ( index >= 200 ? ((index - 200) * v3f(1.0f/200,0,-1.0f/200) + v3f(0,1.0f,1.0f)) :
        ( index * v3f(0,1.0f/200,0) + v3f(0,0,1.0f) ) );
return ans;
}

inline void FEMengine::assignColor(long int vn, Eigen::Vector3f c){
    for(int i = 0;i<3;i++){
        this -> Vertex->at(vn * 6 + 3 + i) =  (5 * c(i) + 5 * this ->color[vn * 3 + i])/10;
        this -> color[vn * 3 + i] = c(i);
    }
}

inline void FEMengine::setColorModeName() {
    switch (this->colorM) {
        case FORCE_X:
            this->colorModeName = colorModeNameBuffer[1];

        case FORCE_Y:
            this->colorModeName = colorModeNameBuffer[2];

        case FORCE_Z:
            this->colorModeName = colorModeNameBuffer[3];

        case VELOCITY_MAGNITUDE:
            this->colorModeName = colorModeNameBuffer[4];

        case VELOCITY_X:
            this->colorModeName = colorModeNameBuffer[5];

        case VELOCITY_Y:
            this->colorModeName = colorModeNameBuffer[6];

        case VELOCITY_Z:
            this->colorModeName = colorModeNameBuffer[7];

        default: //force_magnitude
            this->colorModeName = colorModeNameBuffer[0];
    }
}
void FEMengine::setColorMode(colorMode m)
{
    this ->colorM = m;
    this -> setColorModeName();
}

inline float FEMengine::calcColorMap(long int vn){
switch(this ->colorM){
    case FORCE_X:
        this -> colorModeName = colorModeNameBuffer[1];
        return abs(this -> force.at(vn * 3 + 0));
    case FORCE_Y:
        this -> colorModeName = colorModeNameBuffer[2];
        return abs(this -> force.at(vn * 3 + 1));
    case FORCE_Z:
        this -> colorModeName = colorModeNameBuffer[3];
        return abs(this -> force.at(vn * 3 + 2));
    case VELOCITY_MAGNITUDE:
        this -> colorModeName = colorModeNameBuffer[4];
        return (float)sqrt( pow(velocity.at(vn * 3 + 0),2)
        + pow(velocity.at(vn * 3 + 1),2)
        + pow(velocity.at(vn * 3 + 2),2));
    case VELOCITY_X:
        this -> colorModeName = colorModeNameBuffer[5];
        return abs(this -> velocity.at(vn * 3 + 0));
    case VELOCITY_Y:
        this -> colorModeName = colorModeNameBuffer[6];
        return abs(this -> velocity.at(vn * 3 + 1));
    case VELOCITY_Z:
        this -> colorModeName = colorModeNameBuffer[7];
        return abs(this -> velocity.at(vn * 3 + 2));
    default: //force_magnitude
        this -> colorModeName = colorModeNameBuffer[0];
        return (float)sqrt( pow(force.at(vn * 3 + 0),2)
        + pow(force.at(vn * 3 + 1),2)
        + pow(force.at(vn * 3 + 2),2));

    }
}

void FEMengine::init(){

    this -> FaceNum = this -> Face -> size() / 3;
    this -> VertexNum = this -> Vertex -> size() / 6;
    this -> ElementNum = this -> Element -> size() / 4;
    this -> B.resize(this -> ElementNum);
    this -> volume.resize(this -> ElementNum);
    this -> color.resize(this -> VertexNum * 3);
    int index = 0;
    for(auto &ele: color)
        ele = ++index % 3 == 0 ? 0 : 1.0f;

    this -> force.resize(this -> VertexNum * 3);
    this -> velocity.resize(this -> VertexNum * 3 );

     //those now can be visited as name[]

    if(isSub)
    {
        this -> BoundaryNum = this -> Boundary -> size();
        this -> BoundaryFaceNum.reserve(this -> BoundaryNum );
        this -> positionList.reserve(BoundaryNum/2);
        this -> velocityList.reserve(BoundaryNum/2);
        this -> forceList.reserve(BoundaryNum/2);
        this -> rotationList.reserve(BoundaryNum/2);

        for (auto const &ele: *Boundary)
            BoundaryFaceNum.emplace_back(ele->size());

    }
    this -> setConstitutive(NEOHOOKEAN);
    this -> setMethod(EXPLICIT);
    this -> computeB_Volume();
    std::cout<<"======= FEMengine dump ======"<< std::endl;
    std::cout << "Initialized!"<< std::endl;

}

void FEMengine::computeB_Volume(){
    long int en = 0;
    for(; en< this -> ElementNum; en++){
        auto d = this -> D(en);
        this -> B[en] = d.inverse();
        this -> volume[en] = abs(d.determinant()) /6 ;
    }

}


void FEMengine::computeForce(){
    long int vn = 0;
    long int en = 0;
    Eigen::Matrix3f H, p;
    int a,top;

    for(; vn < this ->VertexNum; vn++){
        auto bodyIndex =  vn * 3;
        this -> force[bodyIndex + 0] = 0;
        this -> force[bodyIndex + 1] = -1 * this -> g;
        this -> force[bodyIndex + 2] = 0;
    }

    for(; en< this -> ElementNum; en++){
        p = this -> P(en);
        H = -this -> volume[en] * (p * (this -> B[en]).transpose());
        top = this->Element->at(en * 4 + 3);

        for(long int  j = 0; j < 3; j++) {
            a = this->Element->at(en * 4 + j);
            colAdd(&(this->force), a, H.col(j));
            colAdd(&(this->force), top, -1 * H.col(j));
        }

    }


}

inline void FEMengine::assignBoundary(bL Ele, vector<float>* data){

    for(auto ele: *(Ele.subFace)) {
        if (ele >-1) {
            data->at(ele * 3 + 0) = Ele.data[0];
            data->at(ele * 3 + 1) = Ele.data[1];
            data->at(ele * 3 + 2) = Ele.data[2];
        }
    }

}
void FEMengine::assignRotationBoundary(rL Ele, vector<float> *data){
    Eigen:: Vector3f a, norm, center, res;
    norm << Ele.norm[0], Ele.norm[1], Ele.norm[2];
    center << Ele.center[0], Ele.center[1], Ele.center[2];

    for(auto ele: *(Ele.subFace)) {
        if(ele > -1){
            a =  node(ele);
            a -= center;
            res = a.cross(norm);
            for(int j = 0;j<3;j++)
                data ->at(ele * 3 + j) = res(j);
        }
    }
}

void FEMengine::handleForce(){
    if (!this->isSub || this->forceList.empty()) //check empty
        return;

    for(auto forceEle : forceList)
        if(forceEle.active)
            assignBoundary(forceEle, &(this -> force));

}
void FEMengine::handleRotation(){
    if (!this->isSub || this->rotationList.empty()) //check empty
        return;

    for(auto rotationEle : rotationList)
        if(rotationEle.active){
            assignRotationBoundary(rotationEle, &(this -> velocity));
        }

}

void FEMengine::handlePosition(){
    if (!this->isSub || this->positionList.empty()) //check empty
        return;

    for(auto positionEle : positionList)
        if(positionEle.active)
        {
            for(auto ele: *(positionEle.subFace)) {
                if (ele >-1) {
                    this -> Vertex->at(ele * 6 + 0) = positionEle.data[0];
                    this -> Vertex->at(ele * 6 + 1) = positionEle.data[1];
                    this -> Vertex->at(ele * 6 + 2) = positionEle.data[2];
                }
            }
        }



}

void FEMengine::handleVelocity(){
    if (!this->isSub || this->velocityList.empty()) //check empty
        return;

    for(auto velocityEle : velocityList)
        if(velocityEle.active) {
            assignBoundary(velocityEle, &(this->velocity));
        }

}

void FEMengine::smooth(long int v1, long int v2, long int v3){
    float c[3];

for(int i = 0; i < 3; i++){
    c[i] = (Vertex -> at(v1 * 6 + 3 + i) +
            Vertex -> at(v2 * 6 + 3 + i) +
            Vertex -> at(v3 * 6 + 3 + i)) / 3;
    this -> Vertex -> at(v1 * 6 + 3 + i) = c[i];
    this -> Vertex -> at(v2 * 6 + 3 + i) = c[i];
    this -> Vertex -> at(v3 * 6 + 3 + i) = c[i];
    this -> color[ v1 * 3 + i] = c[i];
    this -> color[ v2 * 3 + i] = c[i];
    this -> color[ v3 * 3 + i] = c[i];
 }

}

void FEMengine::timeIntegrate(){
    if(method == EXPLICIT) {
        this->computeForce(); //force boundary assignment
        handleForce();
        //force -> velocity
        for (long int vn = 0; vn < this->VertexNum; vn++) {
            for (int i = 0; i < 3; i++) {
                this->velocity.at(vn * 3 + i) *= this->damping; //damping velocity
                this->velocity.at(vn * 3 + i) += this->force.at(vn * 3 + i) / this->nodeMass * this->dt;
            }
        }

        handleVelocity(); // velocity boundary assignment
        handleRotation();
        // velocity -> position
        for (long int vn = 0; vn < this->VertexNum; vn++) {
            for (int i = 0; i < 3; i++) {
                this->Vertex->at(vn * 6 + i) += this->dt * this->velocity.at(vn * 3 + i);

                //coordinate y (height, floor collision detection)
                if (i == 1) {
                    if (this->Vertex->at(vn * 6 + 1) < this->floor) {
                        this->velocity.at(vn * 3 + 1) = rebound * this->velocity.at(vn * 3 + 1);
                        this->Vertex->at(vn * 6 + 1) = this->floor;
                    }
                }
            }
            if (!(runtime % colorFrequent)) {
                auto tmp = calcColorMap(vn);
                auto colorVector = colorMap(tmp);
                assignColor(vn, colorVector);
//
            }
        }
        if (!(runtime % colorFrequent)) {
            for (int i = 0; i < 2; i++)
                for (int fn = 0; fn < FaceNum; fn++) {
                    auto v1 = Face->at(fn * 3 + 0);
                    auto v2 = Face->at(fn * 3 + 1);
                    auto v3 = Face->at(fn * 3 + 2);
                    this->smooth(v1, v2, v3);
                }
        }
        handlePosition(); // vertex position boundary assignment
        std::cout << "======= FEMengine dump ======" << std::endl;
        std::cout << "TimeIntegrate!" << std::endl;
    }
    this -> runtime ++;
}
