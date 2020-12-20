#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec4 Normal;
in vec3 FragPos;

struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
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
#define NR_POINT_LIGHTS 1
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

uniform bool textured;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;


void main()
{    
	vec3 norm = normalize(vec3(Normal));
	vec3 viewDir = normalize(viewPos - FragPos);
	//
	vec3 result = calc_directional(dirlight, norm, viewDir);
	//
	for (int i=0; i<NR_POINT_LIGHTS; i++)
		result += calc_point(pointlights[i], norm, FragPos, viewDir);

	result += calc_spot(spotlight, norm, FragPos, viewDir);

	if (textured)
		FragColor = texture(texture_diffuse1, TexCoords) * vec4(result, 1.0);
	else
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
	
	vec3 ambient  = light.ambient  * material.ambient;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;

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
	
	vec3 ambient  = light.ambient  * material.ambient;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse;
    vec3 specular = light.specular * spec * material.specular;
	
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
	
	vec3 ambient  = light.ambient  * material.ambient * attenuation * intensity;
    vec3 diffuse  = light.diffuse  * diff * material.diffuse * attenuation * intensity;
    vec3 specular = light.specular * spec * material.specular * attenuation * intensity;

	return (ambient + diffuse + specular) * spotlight.color;
}