#version 430 core

layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;
//out vec4 FragColor;

#define MAT_DIFFUSE 0
#define MAT_SPECULAR 1
#define MAT_REFRACT 2

#define AXIS_X 3
#define AXIS_Y 4
#define AXIS_Z 5

#define NOT_INTERSECTED (1.0e6) 
#define PI 3.14159265

struct Camera{
    vec3 position;
    vec3 front;
    vec3 right;
    vec3 up;
    float near;
    float far;
    float zoom;
};  


struct Ray
{
    vec3 o;
    vec3 d;
};

struct Plane
{
    int axis;
    float distanceFromO;

    vec3 color;
    int matType;
    
};

struct HitRecord{
	float t;
	vec3 position;
    vec3 color;
	vec3 normal;
	float matType;
	
};

struct Light{
    vec3 position;
    vec3 color;
    float intensity;
};

//uniform parameter
uniform int objNum;
uniform float width;
uniform float height;

uniform vec2 verticesTexSize;

uniform mat4 shadowTrans;

uniform sampler2D verticesTex;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDepth;
uniform sampler2D gColor;
uniform sampler2D shadowMap;

uniform Camera mainCamera;
uniform Light light;



//const parameter
const int rayTraceDep=10;

//varible parameter
uint m_u = uint(521288629);
uint m_v = uint(362436069);

//最外围墙壁
Plane planes[5];

void initRNG()
{
    uint seed = uint(gl_FragCoord.x) * 1973u
              + uint(gl_FragCoord.y) * 9277u
              + 12345u;
    m_u = seed;
    m_v = seed ^ 0x68bc21ebu; // XOR扰动
}

uint GetUintCore(inout uint u, inout uint v){
	v = uint(36969) * (v & uint(65535)) + (v >> 16);
	u = uint(18000) * (u & uint(65535)) + (u >> 16);
	return (v << 16) + u;
}

float GetUniformCore(inout uint u, inout uint v){
	uint z = GetUintCore(u, v);
	
	return float(z) / uint(4294967295);
}

float GetUniform(){
	return GetUniformCore(m_u, m_v);
}

uint GetUint(){
	return GetUintCore(m_u, m_v);
}

float rand(){
	return GetUniform();
}

vec2 rand2(){
	return vec2(rand(), rand());
}

vec3 random_in_unit_sphere(){
	vec3 p;
	
	float theta = rand() * 2.0 * PI;
	float phi   = rand() * PI;
	p.y = cos(phi);
	p.x = sin(phi) * cos(theta);
	p.z = sin(phi) * sin(theta);
	
	return p;
}

vec3 GammaCorrection(vec3 c){
	return pow(c, vec3(1.0 / 2.2));
}

Plane planeConstruct(int axis, float dis, vec3 color)
{
    Plane p;
    p.axis=axis;
    p.distanceFromO=dis;
    p.color=color;
    p.matType=MAT_DIFFUSE;

    return p;
}

void initWall()
{
    planes[0]=planeConstruct(AXIS_X, 1.5, vec3(0.8,0.3,0.3));
    planes[1]=planeConstruct(AXIS_X, -1.5, vec3(0.3,0.8,0.3));
    planes[2]=planeConstruct(AXIS_Y, 1.5, vec3(0.3,0.3,0.8));
    planes[3]=planeConstruct(AXIS_Y, -1.5, vec3(0.75));
    planes[4]=planeConstruct(AXIS_Z, -5, vec3(0.75));
}

void initRay(out Ray r)
{
    vec2 uv=2.0*(TexCoords+rand2()/vec2(width,height))-1.0;
    float halfNearHeight=mainCamera.near*tan(radians(mainCamera.zoom/2.0));
    float halfNearWidth=(width/height)*halfNearHeight;

    r.o=mainCamera.position;
    r.d=normalize(mainCamera.front*mainCamera.near+
                  mainCamera.right*uv.x*halfNearWidth+
                  mainCamera.up*uv.y*halfNearHeight);
    
}

Ray world2Local(Ray r, mat4 model)
{
    vec3 e=r.o+r.d;

    vec4 localO=inverse(model)*vec4(r.o,1.0);
    localO/=localO.w;

    vec4 localE=inverse(model)*vec4(e,1.0);
    localE/=localE.w;

    Ray localR;
    localR.o=localO.xyz;
    localR.d=normalize(localE.xyz-localO.xyz);

    return localR;
}

////////////////////////////collision detection/////////////////////////
bool pIntersect(Ray r, Plane p, out float t, out vec3 normal)
{
    t=-1.0;
    if(p.axis == AXIS_X) {
  	 	if(r.d.x != 0.0){                         
    		t=(p.distanceFromO - r.o.x) / r.d.x;
            normal=vec3(-sign(p.distanceFromO),0.0,0.0);
        }
  	}
  	else if(p.axis == AXIS_Y) {
  		if(r.d.y != 0.0){
  			t=(p.distanceFromO - r.o.y) / r.d.y;
            normal=vec3(0.0,-sign(p.distanceFromO),0.0);
        }
  	}
  	else if(p.axis == AXIS_Z) {
  		if(r.d.z != 0.0){
  			t=(p.distanceFromO - r.o.z) / r.d.z;
            normal=vec3(0.0,0.0,-sign(p.distanceFromO));
        }
  	}
  	
    if(t<=0.0)
        return false;
    else
        return true;
}

bool hitPlane(Ray r, inout HitRecord rec)
{
    float minT=-1.0f;
    float tempT=-1.0f;
    vec3 tempN;
    for(int i=0;i<5;i++)
    {
        if(pIntersect(r,planes[i],tempT,tempN))
        {
            if(tempT > 0.0 && (minT <= 0.0 || tempT < minT))
            {
                minT=tempT;

                rec.t=minT;
                rec.color=planes[i].color;
                rec.normal=tempN;
                rec.matType=MAT_DIFFUSE;
            }
           
        }
    }
    if(minT>0){
       
        rec.position = r.o + r.d * rec.t;
        // 法线总与入射相反
        if (dot(rec.normal, r.d) > 0.0) rec.normal = -rec.normal;
        // 交点偏移，避免自相交
        rec.position += rec.normal * 1e-3;
        return true;
    }
    else
        return false;
}

bool ifHitTri(vec3 v1, vec3 v2, vec3 v3, vec3 n, Ray r, out float t)
{
     //和三角形所在的面判断交点
    t=dot(v1-r.o,n)/dot(r.d,n);

    if(t<=0.0)
    {
        return false;
    }

    vec3 p=r.o+t*r.d;
    //然后判断是否在三角形内
    vec3 c1=cross(v2-v1,p-v1);
    vec3 c2=cross(v3-v2,p-v2);
    vec3 c3=cross(v1-v3,p-v3);

    if(dot(c1,c2)>0 && dot(c2,c3)>0)
    {
       return true;
    }
    else
    {
        return false;
    }

    
}


bool hitGlasses(Ray r, inout HitRecord rec)
{
    float minT=-1.0f;
    int minObjIdx=-1;
    int minIndicesIdx=-1;
    vec4 minInfo=vec4(-1.0f);
    vec3 minNormal=vec3(0.0f);
    mat4 minModel=mat4(0.0f);
    float tempT=-1.0f;
    for(int i=0;i<objNum;i++)
    {
        //对于glasses[i]
        //获得索引数数量
        vec4 info=texture(verticesTex,vec2(0.5, i+0.5)/verticesTexSize);
        float indicesNum=info.g;
        //获得model矩阵
        mat4 model=mat4(texture(verticesTex,vec2(2.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(3.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(4.5, i+0.5)/verticesTexSize),
                        texture(verticesTex,vec2(5.5, i+0.5)/verticesTexSize));

        //将ray 转换object space
        Ray localR=world2Local(r, model);
       

        //3个一组读取顶点信息进行碰撞检测
        float localMinT=-1.0f; // 对应obj Space 的t
        int localMinIdx=-1;
        vec3 localMinN=vec3(0.0f);
        for(int j=0;j<indicesNum;j+=3)
        {
            float x1=6+j*2+0.5;
            float y=i+0.5;
            vec3 v1=texture(verticesTex,vec2(x1,y)/verticesTexSize).xyz;
            vec3 v2=texture(verticesTex,vec2(x1+2,y)/verticesTexSize).xyz;
            vec3 v3=texture(verticesTex,vec2(x1+4,y)/verticesTexSize).xyz;

            //采样法线
            vec3 n=texture(verticesTex,vec2(x1+1,y)/verticesTexSize).xyz;

            if(ifHitTri(v1,v2,v3,n,localR,tempT))
            {
                if(localMinT<=-1.0f || tempT<localMinT)
                {
                    localMinT=tempT;
                    localMinIdx=j;
                    localMinN=n;
                }
            }
        }

        //将localMinT 转换成 world space 和 minT 做比较 并更新 minT 和 index
        if(localMinT>0.0f) //该物体存在碰撞才有意义
        {
            vec3 localP=localR.o+localMinT*localR.d;
            vec4 p=model*vec4(localP,1.0f);
            p/=p.w;
            float tempT=(p.x-r.o.x)/r.d.x;
            if(minT<=0.0f || tempT<minT)
            {
                minT=tempT; //world
                minObjIdx=i;
                minIndicesIdx=localMinIdx;
                minInfo=info;
                minNormal=localMinN; //local
                minModel=model;      //local
            }
        }
    }

    if(minT <=0.0000001f) //太小说明失败，不用特意更新了
        return false;

    //根据 minT 和 index 采样并插值计算 normal color 等 并填充 hitrecord
    if(rec.t<=0.0f || minT<rec.t) //此时rec 中没有碰撞信息， 或者现在的碰撞信息 比record 中的距离更近，就更新
    {
        rec.t=minT;
	    rec.position=r.o+minT*r.d;
        rec.color=texture(verticesTex,vec2(1.5,minObjIdx+0.5)/verticesTexSize).xyz;
	    rec.normal = normalize(mat3(transpose(inverse(minModel))) * minNormal);
	    rec.matType=minInfo.z;
        return true;
    }

    return false;
}

bool ifHit(Ray r, inout HitRecord rec)
{
    //判断最外面的墙
    hitPlane(r,rec);

    hitGlasses(r,rec);

    if(rec.t>0.0f)
        return true;
    else
        return false;
}

bool ifInShadow(vec3 position)
{
    //计算当前点的的LightSpace 坐标
    vec4 shadowPos=shadowTrans*vec4(position,1.0);
    shadowPos/=shadowPos.w;

    //判断是否在shadowMap 中， 如果不在算能被直接照亮
    shadowPos.xyz=shadowPos.xyz*0.5+0.5; //转换到 [0,1]空间
    if(shadowPos.x<=0.0 ||shadowPos.x>=1.0 || shadowPos.y<=0.0 ||shadowPos.y>=1.0)
        return false;

    //采样shadowMap
    float shadowMapD=texture(shadowMap,shadowPos.xy).x;
    float bias = 0.00005;
    if (shadowPos.z - bias > shadowMapD)
    {
         return true;
    }
    else
    {
        return false;
    }
        
}
/////////////////////////////////////////////////////////////////

/////////////////////////Filter//////////////////////////////////


/////////////////////////////////////////////////////////////////

//////////////////////////Material and Light/////////////////////
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 BRDF_CookTorrance(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);

    // 基础反射率
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // 各个项
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // specular
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3 specular = numerator / denominator;

    // diffuse
    vec3 kd = (1.0 - F) * (1.0 - metallic);
    vec3 diffuse = kd * albedo / PI;

    // final BRDF
    return diffuse + specular;
}

vec3 compute_direct_light_PBR(vec3 lightIntensity, vec3 n, vec3 v, vec3 l, vec3 albedo, float metallic, float roughness) {

    return lightIntensity * BRDF_CookTorrance(n, v, l, albedo, metallic, roughness) * clamp((dot(n, l)), 0.0, 1.0);
}

vec3 BRDF_BlinnPhong(vec3 n, vec3 v, vec3 l, vec3 kd, vec3 ks, float shininess)
{
    vec3 h = normalize(v + l);
    float spec = pow(max(dot(n, h), 0.0), shininess);
    vec3 diffuse = kd / PI;
    vec3 specular = ks * spec;
    return diffuse + specular;
}

vec3 compute_direct_light(vec3 lightIntensity, vec3 n, vec3 v, vec3 l, vec3 kd, vec3 ks, float shininess) {

   
    return lightIntensity * BRDF_BlinnPhong(n, v, l, kd, ks, shininess) * clamp((dot(n, l)), 0.0, 1.0);
}

bool LambertianScatter(vec3 throughput, HitRecord rec, Ray r, out Ray nextR, out vec3 atten, inout vec3 directLight)
{
    vec3 position=r.o+r.d*rec.t;
    vec3 l=normalize(light.position-position);

	nextR.o = position+rec.normal*1e-5;
    vec3 d = normalize(rec.normal + random_in_unit_sphere());
    //if (dot(d, rec.normal) < 0.0) d = -d;
    nextR.d = d;

    //atten = BRDF_BlinnPhong(rec.normal, -r.d, l, rec.color, vec3(0.05),4.0)*clamp(dot(rec.normal,nextR.d), 0.0, 1.0);
    atten = BRDF_CookTorrance(rec.normal, -r.d, l, rec.color, 0.0, 1.0);
    //If not in shadow, update direct Light
    if(!ifInShadow(position))
        //directLight+=throughput*compute_direct_light(light.color*light.intensity, rec.normal, -r.d, l, rec.color, vec3(0.05),4.0);
        directLight+=throughput*compute_direct_light_PBR(light.color*light.intensity, rec.normal, -r.d, l, rec.color, 0.0, 1.0);
    //检查 反射
    //FragColor=vec4(rec.color,1.0);

    return true;

	
}


bool materialScatter(vec3 throughput, HitRecord rec, Ray r,out Ray nextR, out vec3 atten, inout vec3 directLight)
{
    if(rec.matType == MAT_DIFFUSE)
		return LambertianScatter(throughput, rec, r, nextR, atten, directLight);
	else if(rec.matType == MAT_SPECULAR)
		return false;
	else if(rec.matType == MAT_REFRACT)
		return false;
	else
		return false;
}


bool firstJump(out Ray nextR, out vec3 atten, inout vec3 directLight)
{   
   
    //采样法线
    vec3 normal=texture(gNormal,TexCoords).xyz;
    //采样世界空间
    vec3 position=texture(gPosition,TexCoords).xyz;

    vec4 color_mat=texture(gColor,TexCoords);
   
    if(color_mat.a == MAT_DIFFUSE)
    {
        vec3 v=normalize(mainCamera.position-position);
        vec3 l=normalize(light.position-position);

        nextR.o = position+normal*1e-5;
        vec3 d = normalize(normal + random_in_unit_sphere());
        nextR.d = d;

        //采样颜色
        //atten=BRDF_BlinnPhong(normal, v, l, color_mat.xyz, vec3(0.05),4.0);
        atten=BRDF_CookTorrance(normal, v, l, color_mat.xyz, 0.0, 1.0);
        //FragColor=vec4(atten,1.0);

        //如果在shadow 中
        if(!ifInShadow(position))
            //directLight+=compute_direct_light(light.color*light.intensity, normal, v, l, color_mat.xyz, vec3(0.05),4.0);
            directLight+=compute_direct_light_PBR(light.color*light.intensity, normal, v, l, color_mat.xyz, 0.0, 1.0);
        FragColor=vec4(directLight,1.0);
        return true;
    }

    return false;
    
}


/////////////////////////////////////////////////////////////////

void main()
{
    
    initRNG();
    //初始化碰撞盒
    initWall();

    HitRecord rec;
    Ray r;
    vec3 rayTrColor=vec3(0.0f);
    vec3 bgColor=vec3(30.0f);
    vec3 throughput=vec3(1.0f);

    //init ray dir according to the Texcoords
    //initRay(r);
    
    //first Jump by gBuffer
    firstJump(r, throughput, rayTrColor);

    //FragColor=vec4((r.d.x),0.0,0.0,1.0);
    for(int i=1;i<rayTraceDep;i++)
    {
        rec.t=-1.0f; //清空上一次的反射结果       
        
        //begin ray Tracing
        if(ifHit(r,rec))
        {
            //根据表面属性，重新计算光线方向，能量，或者计算光照进行返回
            vec3 atten;
            Ray nextR;
            if(!materialScatter(throughput, rec, r, nextR, atten, rayTrColor))
            {
                break;
            }
            r = nextR;
			throughput *= atten;
        }
        else
        {
            //vec3 dir = normalize(r.d);
            //float t = 0.5 * (dir.y + 1.0);
            //bgColor = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
            break;
        }
            
    }
    

    FragColor=vec4(rayTrColor,1.0);
    
    //FragColor=vec4(0.2,0.3,0.0,1.0);

    

}