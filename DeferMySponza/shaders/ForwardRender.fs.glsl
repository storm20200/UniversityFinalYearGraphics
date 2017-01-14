﻿#version 450

/// Contains the properties of the material to be applied to the current fragment.
struct Material
{
    float   smoothness;     //!< Effects the distribution of specular light over the surface.
    float   reflectance;    //!< Effects the fresnel effect of dieletric surfaces.
    float   conductivity;   //!< Conductive surfaces absorb incoming light, causing them to be fully specular.
    float   transparency;   //!< How transparent the surface is.
    
    vec3    albedo;         //!< The base colour of the material.
    vec3    normalMap;      //!< The normal map of the material.
} material;

layout (std140) uniform Scene
{
    mat4 projection;    //!< The projection transform which establishes the perspective of the vertex.
    mat4 view;          //!< The view transform representing where the camera is looking.

    vec3 camera;        //!< Contains the position of the camera in world space.
    vec3 ambience;      //!< The ambient lighting in the scene.
} scene;

        in  vec3    worldPosition;  //!< The fragments position vector in world space.
        in  vec3    worldNormal;    //!< The fragments normal vector in world space.
        in  vec3    baryPoint;      //!< The barycentric co-ordinate of the current fragment, useful for wireframe rendering.
        in  vec2    texturePoint;   //!< The interpolated co-ordinate to use for the texture sampler.
flat    in  uint    materialID;     //!< Used in fetching instance-specific data from the uniforms.

        out vec4    fragmentColour; //!< The calculated colour of the fragment.


/// External functions.
void setFragmentMaterial (const in vec2 uvCoordinates, const in float materialID);
vec3 directionalLightContributions (const in vec3 normal, const in vec3 view);
vec3 pointLightContributions (const in vec3 position, const in vec3 normal, const in vec3 view);
vec3 spotlightContributions (const in vec3 position, const in vec3 normal, const in vec3 view);


/**
    Iterates through each light, calculating its contribution to the current fragment.
*/
void main()
{
    // Calculate the required lighting components.
    const vec3 Q = worldPosition;
    const vec3 N = normalize (worldNormal);
    const vec3 V = normalize (scene.camera - Q);

    // Retrieve the material properties and use it for lighting calculations.
    setFragmentMaterial (texturePoint, materialID);

    // Accumulate the contribution of every light.
    const vec3 lighting =   directionalLightContributions (N, V) +
                            pointLightContributions (Q, N, V) +
                            spotlightContributions (Q, N, V);
    
    // Put the equation together and we get...
    const vec3 colour = scene.ambience + lighting;
    
    // Output the calculated fragment colour.
    fragmentColour = vec4 (colour, material.transparency);
}