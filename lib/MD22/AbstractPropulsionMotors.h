/*
 * AbstractMotors.h
 *
 *  Created on: 22 juin 2013
 *      Author: mythril
 */

#ifndef ABSTRACT_PROPULSION_MOTORS_H_
#define ABSTRACT_PROPULSION_MOTORS_H_

#include <config.h>

class AbstractPropulsionMotors {
public:
	// Constantes pour la configuration des moteurs
	#define ASSIGN_UNDEF_MOTOR		0
	#define ASSIGN_MOTOR_1			1
	#define ASSIGN_MOTOR_2			2

	AbstractPropulsionMotors();
	virtual ~AbstractPropulsionMotors();

	void assignMotors(int numMoteurGauche, int numMoteurDroit);
	void generateMouvement(int gauche, int droit);
	void moteurGauche(int cmd);
	void moteurDroit(int cmd);
	void stopGauche();
	void stopDroit();

	void stopAll();

	// Méthode "héritable" //
	// ------------------- //
	virtual void stop1();
	virtual void stop2();

#if defined(DEBUG)
	virtual void printVersion();
#endif

	// Méthodes abstraites //
	// ------------------- //
	virtual void init() = 0;
	virtual void moteur1(int cmd) = 0;
	virtual void moteur2(int cmd) = 0;

protected:
	int minVal;
	int maxVal;

	int prevM1;
	int prevM2;

	int check(int);

private:
	char numMoteurGauche, numMoteurDroit;

	bool alternate;
};

#endif /* ABSTRACT_PROPULSION_MOTORS_H_ */
