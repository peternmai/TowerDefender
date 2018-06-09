#version 330 core

in vec3 vertColor;
in vec3 vertNormal;
in vec3 vertPosition;

out vec4 fragColor;

void main(void) {
    fragColor = vec4(vertColor, 1.0);

	// Information about directional light
	vec3 lightDirection = normalize(vec3(1.0f, -0.5f, -0.5f));
	vec3 lightColor = vec3(1.0f);
	float ambientCoefficent = 0.2;
	float diffuseCoefficient = 2.0;
	float specularCoefficient = 0.1;
	float shininessConstant = 128;

	// Set ambient component
	vec3 ambientColor = vertColor * ambientCoefficent;

	// Set diffuse component
	lightDirection = normalize(-1.0f * lightDirection);
	float diff = diffuseCoefficient * max(dot(lightDirection, vertNormal), 0.0f);
	vec3 diffuseColor = diff * lightColor;

	// Set the specular component
	vec3 reflectionDirection = reflect(-1.0f * lightDirection, vertNormal);
	float spec = pow(max(dot(reflectionDirection, normalize(-1.0f * vertPosition)), 0.0f), shininessConstant);
	vec3 specularColor = specularCoefficient * spec * lightColor;

	// Set color of vertex
	fragColor = vec4((ambientColor + diffuseColor + specularColor) * vertColor, 1.0f);
}