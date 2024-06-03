#!/usr/bin/env python3
"""
Source article:
  https://www.sciencedirect.com/science/article/pii/S0378778823009477
"""

ONE_SECOND_MS = 1000
ONE_MINUTE_MS = ONE_SECOND_MS * 60


class PIController:
    def __init__(
        self,
        Kp: float,
        Ti: float,
        db: int = 0,
        Voamin: float = float("NaN"),
        Voamax: float = float("NaN"),
    ) -> None:
        # proportional gain [L/s.ppm-CO2]
        self.kp = Kp
        # integral gain [min]
        self.ti = Ti
        # dead band [ppm]
        self.db = db
        # minimum outdoor airflow rate [L/s]
        self.v_oa_min = Voamin
        # maximum outdoor airflow rate [L/s]
        self.v_oa_max = Voamax

        self._ib = 0.0
        self._dt0 = 0
        self.now = 0  # in ms

    # time step [s]
    def _dt_s(self):
        res = self.now - self._dt0 if self._dt0 != 0 else 0
        self._dt0 = self.now
        return res / ONE_SECOND_MS

    # Cset - CO2 setpoint [ppm]
    # Cval - CO2 concentration [ppm]
    def update1(self, setpoint: int, current: int) -> float:
        # 7: error from setpoint [ppm]
        e: int = setpoint - current

        # 8: error from setpoint, including dead band [ppm]
        e_db: float
        if current < setpoint - self.db:
            e_db = e - self.db
        elif current > setpoint + self.db:
            e_db = e + self.db
        else:
            e_db = 0

        # 9: integral error [min.ppm-CO2]
        ie: float = self._ib + (self._dt_s() / 60.0) * e_db

        # 10: candidate outdoor airflow rate [L/s]
        v_oa_c: float = -self.kp * (e_db + ie / self.ti)

        # 11: integral error with anti-integral windup [min.ppm-CO2]
        if v_oa_c < self.v_oa_min:
            self._ib = -self.ti * (e_db + self.v_oa_min / self.kp)
        elif v_oa_c > self.v_oa_max:
            self._ib = -self.ti * (e_db + self.v_oa_max / self.kp)
        else:
            self._ib = ie

        # 12: outdoor airflow rate [L/s]
        v_oa: float = -self.kp * (e_db + self._ib / self.ti)

        # print(f"ti={self.ti}, ie={ie}, ib={self._ib}, voa={v_oa}")

        return v_oa

    def update2(self, setpoint: int, current: int) -> float:
        error = setpoint - current
        kp = self.kp
        ki = self.ti / 60 / 1000
        dt = self._dt_s() * 1000

        proportional_term = kp * error
        new_integral = error * dt * ki

        accumulated_integral = self._ib + new_integral
        self._ib = accumulated_integral

        integral_term = accumulated_integral

        derivative_term = 0

        output = proportional_term + integral_term + derivative_term

        e_db = error
        ie = integral_term
        v_oa_c = output
        v_oa = v_oa_c

        print(
            ""
            # f"dt={dt}," +
            # f"now={int(now/1000/60):3}, "
            + f"e={error:4}, Edb={int(e_db):4},"
            + f" sp={setpoint}, ppm={current:3}, Voac={v_oa_c:8.02f},"
            # + f" Voamin={Voamin_:.2f}, Voamax={Voamax_:.2f},"
            # + f" Doac={Dat.cdoa(Voac):d}, cspd={Dat.cspd(Voac):d},"
            + f" I={ie:8.2f},"
            # + f" Ib={Ib:8.2f} {Ibv},"
            # + f" Voa={Voa:5.2f},"
            # + f" Doa={Dat.cdoa(Voa):3},"
            # + f" spd={Dat.cspd(Voa)}"
        )

        return output

    def update(self, setpoint: int, current: int) -> float:
        return self.update1(setpoint, current)


class Gen:
    def __init__(self, setpoint, t_step, pi: list[tuple[str, PIController]]) -> None:
        self.tm = 0
        self.data: list[dict] = []
        self.setpoint = setpoint
        self.t_step = t_step
        self.ppm = self.setpoint
        self.pi = pi

    def rng(self, begin, end, step):
        print(f"from {begin} to {end} by {step}")
        if begin < end:
            return range(begin, end + 1, step)
        return reversed(range(end + 1, begin, step))

    def upd(self):
        self.tm += self.t_step
        dat = {"tm": self.tm, "ppm": self.ppm}
        for key, pi in self.pi:
            pi.now = self.tm
            rate = pi.update(self.setpoint, self.ppm)
            dat[key] = rate

        # self.data.append(Dat(self.tm, self.ppm, rate))
        self.data.append(dat)

    def gen(self, begin, end, step=3):
        for ppm in self.rng(begin, end, step):
            self.ppm = ppm
            self.upd()

    def sleep(self, minutes):
        print(f"sleep for {minutes} min")
        for _ in range(minutes):
            self.upd()


def run_test(setpoint, pi: list[tuple[str, PIController]], data: list[tuple[int, int]]):
    pa4s = [0, 30, 45, 60, 75, 90, 120]
    paLt = [0, 20, 30, 40, 50, 60, 80]
    pa3s = [0, 15, 30, 50, 60, 75, 100]
    paO2 = [0, 35, 60, 75, 120]
    pa = pa4s

    Voa_min = pa[0]
    Voa_max = pa[6]

    print(f"Voamin={Voa_min:.2f}, Voamax={Voa_max:.2f}")

    for k, c in pi:
        c.v_oa_min = Voa_min
        c.v_oa_max = Voa_max
        print(f"{k}: kp={c.kp}")

    g = Gen(setpoint=setpoint, t_step=ONE_MINUTE_MS, pi=pi)

    start = 0
    for ppm, tm in data:
        if start != 0:
            g.gen(start, ppm)
            g.sleep(tm)
        start = ppm

    return g.data


if __name__ == "__main__":
    run_test()
