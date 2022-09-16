float RAD = pow((sin(iTime) + 1) / 2, 2);

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
	float res = max(iResolution.x, iResolution.y);
	vec2 scsp = (fragCoord / res) * 2 - vec2(1, 1);
	float len = pow(scsp.x, 2) + pow(scsp.y, 2);
	if(len < RAD) fragColor = vec4(1, 1, 1, 1);
	else discard;
}