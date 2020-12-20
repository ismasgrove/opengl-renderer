#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

struct Material {
	sampler2D diffuse;
	sampler2D specular;
	sampler2D emission;
	float shininess;
};
uniform Material material;

struct Dirlight {
	vec3 direction, ambient, diffuse, specular, color;
};
uniform Dirlight dirlight;

struct Pointlight {
	vec3 position, ambient, diffuse, specular, color;
	float constant, linear, quadratic;
};
#define NR_POINT_LIGHTS 3
uniform Pointlight pointlights[NR_POINT_LIGHTS];

struct Spotlight {
	vec3 position, direction, ambient, diffuse, specular,  color;
	float innerCutoff, outerCutoff, constant, linear, quadratic;
};
uniform Spotlight spotlight;

uniform vec3 viewPos;

vec3 calc_directional(Dirlight light, vec3 normal, vec3 viewDir);
vec3 calc_point(Pointlight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 calc_spot(Spotlight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
	vec3 norm = normalize(Normal);
	vec3 viewDir = normalize(viewPos - FragPos);
	//
	vec3 result = calc_directional(dirlight, norm, viewDir);
	//
	for (int i=0; i<NR_POINT_LIGHTS; i++)
		result += calc_point(pointlights[i], norm, FragPos, viewDir);

	result += calc_spot(spotlight, norm, FragPos, viewDir);

	FragColor = vec4(result, 1.0);
}

vec3 calc_directional(Dirlight light, vec3 normal, vec3 viewDir)
{
	vec3 lightDir = normalize(-light.direction);
	//
	float diff = max(dot(lightDir, normal), 0.0);
	//
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(reflectDir, viewDir), 0.0), material.shininess);
	//
	vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

    return (ambient + diffuse + specular) * light.color;
}

vec3 calc_point(Pointlight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	//
	float diff = max(dot(lightDir, normal), 0.0);
	//
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(lightDir, reflectDir), 0.0), material.shininess);
	//
	vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));
	//
	float dist = distance(light.position, fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * dist + 
  			     light.quadratic * (dist * dist));
	//
	return (ambient + diffuse + specular) * attenuation * light.color;
}

vec3 calc_spot(Spotlight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 lightDir = normalize(light.position - fragPos);
	//
	float theta = dot(lightDir, normalize(-light.direction));
	float epsilon = light.innerCutoff - light.outerCutoff;
	float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
	//
	float diff = max(dot(lightDir, normal), 0.0);
	//
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	//
	float dist = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * dist + 
  			     light.quadratic * (dist * dist));
	//
	vec3 ambient  = light.ambient  * vec3(texture(material.diffuse, TexCoords));
    vec3 diffuse  = light.diffuse  * diff * vec3(texture(material.diffuse, TexCoords));
    vec3 specular = light.specular * spec * vec3(texture(material.specular, TexCoords));

	ambient *= attenuation * intensity;
	diffuse *= attenuation * intensity;
	specular *= attenuation * intensity;

	return (ambient + diffuse + specular) * spotlight.color;
}