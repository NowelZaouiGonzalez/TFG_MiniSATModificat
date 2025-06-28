/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>
#include <stdexcept>
#include <queue>
#include <deque>
#include <fstream> // Per crear l'arxiu reduccio.txt

#include "mtl/Sort.h"
#include "core/Solver.h"

using namespace Minisat;

//=================================================================================================
// Options:

static const char *_cat = "CORE";

static DoubleOption opt_var_decay(_cat, "var-decay", "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
static DoubleOption opt_clause_decay(_cat, "cla-decay", "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
static DoubleOption opt_random_var_freq(_cat, "rnd-freq", "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption opt_random_seed(_cat, "rnd-seed", "Used by the random variable selection", 91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption opt_ccmin_mode(_cat, "ccmin-mode", "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption opt_phase_saving(_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption opt_rnd_init_act(_cat, "rnd-init", "Randomize the initial activity", false);
static BoolOption opt_luby_restart(_cat, "luby", "Use the Luby restart sequence", true);
static IntOption opt_restart_first(_cat, "rfirst", "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption opt_restart_inc(_cat, "rinc", "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption opt_garbage_frac(_cat, "gc-frac", "The fraction of wasted memory allowed before a garbage collection is triggered", 0.20, DoubleRange(0, false, HUGE_VAL, false));

/// MY OPTIONS
static IntOption opt_initial_polarity(_cat, "init-pol", "Controla quin valor inicial de polaritat del literal es vol (0=Per Defecte, 1=True, 2=False)", 0, IntRange(0, 2));
static IntOption opt_initial_seed_doit(_cat, "ini-seed", "Per donar un valor inicial a la seed per la funcio doIT", 23061912, IntRange(1, INT32_MAX));
static BoolOption opt_ba(_cat, "aBinaries", "Indica fer analisis de clausules binaries", false);
static BoolOption opt_ta(_cat, "aTernaries", "Indica fer analisis de clausules Ternaries. Activa tambe analisis de les binaries", false);
static BoolOption opt_ci(_cat, "see-impl", "Indica fer cerca de implicacions. Activa tambe analisis de les binaries. Si no s'estableix la quantitat, per defecte es fara nomes un sol cop.", false);
static BoolOption opt_opt_std(_cat, "opt-impl", "Indica optimitzar l'estructura de dades de les binaries", false);
static StringOption opt_quantCercaImpl(_cat, "ncerc", "Indica cada quant vols fer la mirar les implicacions (O=[O]ne Time , A=[A]llways, (1..9)(0..9)* = Numero de vegades). Activara fer la cerca d'implicacions", "");
static BoolOption opt_er(_cat, "e-er", "Indica voler fer l'estrategia per evitar restarts amb unitaries (ER).", false);
static IntOption opt_per(_cat, "pe-er", "Indica el percentatge dessitgat que es vol per fer l'estrategia ER. Assignar percentatge (0,100] activa l'estrategia. Si nomes s'activa ER, el percentatge per defecte es 100", 0, IntRange(0, 100)); // Posar 0, no acaba activant l'estrategia.
static BoolOption opt_rac(_cat, "r-ac", "Indica voler fer l'estrategia per evitar retroces excesiu (AC).", false);
static IntOption opt_prac(_cat, "pr-ac", "Indica el percentatge dessitgat que es vol per fer l'estrategia AC. Assignar percentatge (0,100] activa l'estrategia. Si nomes s'activa AC, el percentatge per defecte es 100", 0, IntRange(0, 100)); // Posar 0, no acaba activant l'estrategia.
static BoolOption opt_mirar_reducc(_cat,"mirar-reduccio","Indica per mirar de mostrar la reduccio de la base de dades despres de la primera simplificacio",false);
// END MY OPTIONS

//=================================================================================================
// Constructor/Destructor:

Solver::Solver() :

                   // Parameters (user settable):
                   //
                   verbosity(0), var_decay(opt_var_decay), clause_decay(opt_clause_decay), random_var_freq(opt_random_var_freq), random_seed(opt_random_seed), luby_restart(opt_luby_restart), ccmin_mode(opt_ccmin_mode), phase_saving(opt_phase_saving), rnd_pol(false), rnd_init_act(opt_rnd_init_act), garbage_frac(opt_garbage_frac), restart_first(opt_restart_first), restart_inc(opt_restart_inc)

                   // Parameters (the rest):
                   //
                   ,
                   learntsize_factor((double)1 / (double)3), learntsize_inc(1.1)

                   // Parameters (experimental):
                   //
                   ,
                   learntsize_adjust_start_confl(100), learntsize_adjust_inc(1.5)

                   // Statistics: (formerly in 'SolverStats')
                   //
                   ,
                   solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0), dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

                   ,
                   ok(true), cla_inc(1), var_inc(1), watches(WatcherDeleted(ca)), qhead(0), simpDB_assigns(-1), simpDB_props(0), order_heap(VarOrderLt(activity)), progress_estimate(0), remove_satisfied(true)

                   // Resource constraints:
                   //
                   ,
                   conflict_budget(-1), propagation_budget(-1), asynch_interrupt(false)
{
    // MY VARIABLES
    // Es troben en protected. Per no moure-ho a public, es fa aqui
    fer_mirar_reduccio=opt_mirar_reducc;
    v_init_pol = opt_initial_polarity;
    seed_for_doit = (double)opt_initial_seed_doit;

    mirar_ternaries = opt_ta;
    mirar_All_implications = opt_ci;

    opcioCerca = opt_quantCercaImpl;

    contador_quants_cops_cerca_implicacio = 0;

    if (opcioCerca != "")
        mirar_All_implications = true;

    if (mirar_All_implications)
    {
        if (opcioCerca == "" || opcioCerca == "O")
            MAX_COPS = -1; // Si posem <0, mai executara. Si contador es 0 i max es <0, faras que nomes s'executi una vegada
        else if (opcioCerca == "A")
            MAX_COPS = 0;
        else
        {
            try
            {
                size_t pos;
                int numero = std::stoi(opcioCerca, &pos);
                if (pos == opcioCerca.length()) // Es un numero
                {
                    MAX_COPS = numero;
                }
                else // No es un numero (te caracters no numerics)
                {
                    MAX_COPS = -1;
                }
            }
            catch (const std::invalid_argument &)
            {
                MAX_COPS = -1;
            }
            catch (const std::out_of_range &)
            {
                MAX_COPS = -1;
            }
        }
    }

    fer_preStructure = opt_ba || mirar_ternaries || mirar_All_implications; // Si una esta activa, fer_preStructure ha d'estar-ho tambe

    optimitzar = opt_opt_std;

    evitarRestartsExcesius = opt_er;
    pec = opt_per;
    if (evitarRestartsExcesius) // Hem activat
    {
        if (pec == 0)  // No hem cridat l'opcio o si pero assignant un 0. Si activem l'estrategia, el percentatge es 100
            pec = 100; // Per defecte, agafara aquest valor
        // else esta entre 1 i 100
    }
    else // NO hem activat
    {
        if (pec > 0)                       // Hem assignat percentatge. Activem l'estrategia.
            evitarRestartsExcesius = true; // El percentatge sera l'escollit
    }

    evitarRetroses_AC_Anterior = opt_rac;
    prac = opt_prac;

    if (evitarRetroses_AC_Anterior) // Hem activat
    {
        if (prac == 0)  // No hem cridat l'opcio o si pero assignant un 0. Si activem l'estrategia, el percentatge es 100
            prac = 100; // Per defecte, agafara aquest valor
        // else esta entre 1 i 100
    }
    else // NO hem activat
    {
        if (prac > 0)                          // Hem assignat percentatge. Activem l'estrategia.
            evitarRetroses_AC_Anterior = true; // El percentatge sera l'escollit
    }
}
Solver::~Solver()
{
}

//=================================================================================================
// Minor methods:

// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches.init(mkLit(v, false));
    watches.init(mkLit(v, true));
    assigns.push(l_Undef);
    vardata.push(mkVarData(CRef_Undef, 0));
    // activity .push(0);
    activity.push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen.push(0);
    // MY CODE
    if (v_init_pol == 1) // Volem que La polaritat doni el literal inicial com a true. sign ha de ser false
        sign = false;
    else if (v_init_pol == 2) // Volem que La polaritat doni el literal inicial com a false. sign ha de ser true
        sign = true;
    // else ( v_init_pol == 0). Agafa el que ja arriba per la funcio
    // END MY CODE
    polarity.push(sign);
    decision.push();
    trail.capacity(v + 1);
    setDecisionVar(v, dvar);
    return v;
}

bool Solver::addClause_(vec<Lit> &ps)
{
    assert(decisionLevel() == 0);
    if (!ok)
        return false;

    //*Mirem com son les originals que rep*//
    if(fer_mirar_reduccio)
    {
        if (ps.size() == 2)
            original_binaries_count++;
        else if (ps.size() == 3)
            original_ternaries_count++;
    }

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p, A, B;
    int i, j;
    // int a, b, na, nb;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    ps.shrink(i - j);

    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1)
    {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    }
    else if (fer_preStructure && ps.size() == 2)
    {
        A = ps[0];
        B = ps[1];
        // a = toInt(A);
        // b = toInt(B);
        // na = toInt(~A);
        // nb = toInt(~B);

        if (not canAddPre(A, B))
        {
            uncheckedEnqueue(A);
            ha_simplificat_a_unitaria++;
            return ok = (propagate() == CRef_Undef);
        }
        else if (not canAddPre(B, A))
        {
            uncheckedEnqueue(B);
            ha_simplificat_a_unitaria++;
            return ok = (propagate() == CRef_Undef);
        }
        // else
        if (addPre(A, B)) // Si no ha crescut, es que ja hi havia la binaria
        {
            CRef cr = ca.alloc(ps, false);
            clauses.push(cr);
            attachClause(cr);
            noves_binaries = true;
        }
    }
    else // fer_preStructure==true && ps.size>2 o fer_preStructure==false && ps.size>1
    {
        CRef cr = ca.alloc(ps, false);
        clauses.push(cr);
        attachClause(cr);
    }

    return true;
}

void Solver::attachClause(CRef cr)
{
    const Clause &c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt())
        learnts_literals += c.size();
    else
        clauses_literals += c.size();
}

void Solver::detachClause(CRef cr, bool strict)
{
    const Clause &c = ca[cr];
    assert(c.size() > 1);

    if (strict)
    {
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }
    else
    {
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt())
        learnts_literals -= c.size();
    else
        clauses_literals -= c.size();
}

void Solver::removeClause(CRef cr)
{
    Clause &c = ca[cr];
    detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c))
        vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    ca.free(cr);
}

bool Solver::satisfied(const Clause &c) const
{
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false;
}

// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level)
{
    if (decisionLevel() > level)
    {
        for (int c = trail.size() - 1; c >= trail_lim[level]; c--)
        {
            Var x = var(trail[c]);
            assigns[x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x);
        }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    }
}

//=================================================================================================
// Major methods:

Lit Solver::pickBranchLit()
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty())
    {
        next = order_heap[irand(random_seed, order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++;
    }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty())
        {
            next = var_Undef;
            break;
        }
        else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}

/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit> &out_learnt, int &out_btlevel)
{
    int pathC = 0;
    Lit p = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push(); // (leave room for the asserting literal)
    int index = trail.size() - 1;

    do
    {
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause &c = ca[confl];

        if (c.learnt())
            claBumpActivity(c);

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++)
        {
            Lit q = c[j];

            if (!seen[var(q)] && level(var(q)) > 0)
            {
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if (level(var(q)) >= decisionLevel())
                    pathC++;
                else
                    out_learnt.push(q);
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])])
            ;
        p = trail[index + 1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    } while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2)
    {
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
            if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level))
                out_learnt[j++] = out_learnt[i];
    }
    else if (ccmin_mode == 1)
    {
        for (i = j = 1; i < out_learnt.size(); i++)
        {
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else
            {
                Clause &c = ca[reason(var(out_learnt[i]))];
                for (int k = 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0)
                    {
                        out_learnt[j++] = out_learnt[i];
                        break;
                    }
            }
        }
    }
    else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else
    {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = p;
        out_btlevel = level(var(p));
    }

    for (int j = 0; j < analyze_toclear.size(); j++)
        seen[var(analyze_toclear[j])] = 0; // ('seen[]' is now cleared)
}

// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels)
{
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0)
    {
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause &c = ca[reason(var(analyze_stack.last()))];
        analyze_stack.pop();

        for (int i = 1; i < c.size(); i++)
        {
            Lit p = c[i];
            if (!seen[var(p)] && level(var(p)) > 0)
            {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0)
                {
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }
                else
                {
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}

/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit> &out_conflict)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size() - 1; i >= trail_lim[0]; i--)
    {
        Var x = var(trail[i]);
        if (seen[x])
        {
            if (reason(x) == CRef_Undef)
            {
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }
            else
            {
                Clause &c = ca[reason(x)];
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);
}

/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate()
{
    CRef confl = CRef_Undef;
    int num_props = 0;
    watches.cleanAll();

    while (qhead < trail.size())
    {
        Lit p = trail[qhead++]; // 'p' is enqueued fact to propagate.
        vec<Watcher> &ws = watches[p];
        Watcher *i, *j, *end;
        num_props++;

        for (i = j = (Watcher *)ws, end = i + ws.size(); i != end;)
        {
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True)
            {
                *j++ = *i++;
                continue;
            }

            // Make sure the false literal is data[1]:
            CRef cr = i->cref;
            Clause &c = ca[cr];
            Lit false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit first = c[0];
            Watcher w = Watcher(cr, first);
            if (first != blocker && value(first) == l_True)
            {
                *j++ = w;
                continue;
            }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False)
                {
                    c[1] = c[k];
                    c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause;
                }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False)
            {
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            }
            else
                uncheckedEnqueue(first, cr);

        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}

/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt
{
    ClauseAllocator &ca;
    reduceDB_lt(ClauseAllocator &ca_) : ca(ca_) {}
    bool operator()(CRef x, CRef y)
    {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity());
    }
};
void Solver::reduceDB()
{
    int i, j;
    double extra_lim = cla_inc / learnts.size(); // Remove any clause below this activity

    sort(learnts, reduceDB_lt(ca));
    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // and clauses with activity smaller than 'extra_lim':
    for (i = j = 0; i < learnts.size(); i++)
    {
        Clause &c = ca[learnts[i]];
        if (c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim))
            removeClause(learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
    checkGarbage();
}

void Solver::removeSatisfied(vec<CRef> &cs)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++)
    {
        Clause &c = ca[cs[i]];
        if (satisfied(c))
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}

/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    // MY CODE
    if (fer_preStructure) // Si no fem prestructure, l'algoritme es igual al orignial
        return mysimplify();

    assert(decisionLevel() == 0);

    if (!ok || propagate() != CRef_Undef)
        return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied) // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props = clauses_literals + learnts_literals; // (shouldn't depend on stats really, but it will do for now)

    return true;
}

/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    int backtrack_level;
    int conflictC = 0;
    vec<Lit> learnt_clause;
    starts++;

    for (;;)
    {
        CRef confl = propagate();
        if (confl != CRef_Undef)
        {
            // CONFLICT
            conflicts++;
            conflictC++;
            if (decisionLevel() == 0)
                return l_False;

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level);
            // cancelUntil(backtrack_level);

            //*MY CODE. AQUI APREN LES CLAUSULES

            // Per poder veure quants cops ha trobat binaries i ternaries
            if (learnt_clause.size() == 2)
                ha_aparegut_binaria_learnt++;
            else if (learnt_clause.size() == 3)
                ha_aparegut_ternaria_learnt++;

            if (learnt_clause.size() == 1) // LA CLAUSULA APRESA ES DE MIDA 1
            {
                ha_aparegut_unitaria_learnt++;
                cancelUntil(backtrack_level); // backtrack_level es 0
                uncheckedEnqueue(learnt_clause[0]);
            }
            else if (not fer_preStructure) // Per que no avançi tant entre els else if si hem desconectat analitzar. A veure si aixo minimitza una mica el temps de cpu
            {
                cancelUntil(backtrack_level);

                CRef cr = ca.alloc(learnt_clause, true);
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
            }
            else if (learnt_clause.size() == 2) // LA CLAUSULA APRESA ES DE MIDA 2. fer_preStructure es true
            {
                Lit A, B;
                int a, b;
                int na, nb;

                A = learnt_clause[0];
                a = toInt(A);
                na = toInt(~A);
                B = learnt_clause[1];
                b = toInt(B);
                nb = toInt(~B);

                assert(canAddPreDirect(nb, na) == true); // Mai hauriem de trobar (B)

                if (not canAddPreDirect(na, nb)) // CanAddPre(A,B) Haurem de posar (A)
                {
                    in_binaria_learnt_ha_trobat_A++;

                    if (evitarRestartsExcesius && doIT(pec)) // Si decideix fer l'estrategia, es fara la quantitat de vegades indicada per el percentatge pec
                    {
                        // No posar noves_binaries. Com sabem que es una unitaria, aquesta info es perdra
                        unitariesPerRestart.insert(a); // Ens guardem la unitaria

                        cancelUntil(backtrack_level); // No tornara al inici (probablament)

                        CRef cr = ca.alloc(learnt_clause, true); // Com si no hageim descobert res
                        learnts.push(cr);
                        attachClause(cr);
                        claBumpActivity(ca[cr]);
                        uncheckedEnqueue(learnt_clause[0], cr);
                    }
                    else
                    {
                        cancelUntil(0);      // Que torni al level 0
                        uncheckedEnqueue(A); // Ha de posar A obligatoriament, no nomes la clausula (A B). Es basicament uncheckedEnqueue(learnt_clause[0]);
                    }
                }
                else
                {
                    if (addPreDirect(a, na, b, nb)) // addPre(A,B)
                    {
                        noves_binaries = true;
                        ha_apres_binaria_learnt_nova++;
                    }
                    else
                    {
                        ha_apres_binaria_learnt_ja_existent++;
                    }

                    cancelUntil(backtrack_level); // No tornara al inici (probablament)

                    CRef cr = ca.alloc(learnt_clause, true);
                    learnts.push(cr);
                    attachClause(cr);
                    claBumpActivity(ca[cr]);
                    uncheckedEnqueue(learnt_clause[0], cr);
                }
            }
            else if (mirar_ternaries && learnt_clause.size() == 3) // fer_preStructure es true
            {
                analyze_ternaia_learnt(learnt_clause, backtrack_level); // Es una funcio propia, ja que no es el mateix analitzar yuna ternaria ara creada que a nivell 0
            }
            else // fer_preStructure==true && size>3
            {
                cancelUntil(backtrack_level);

                CRef cr = ca.alloc(learnt_clause, true);
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
            }
            /// END MY CODE

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0)
            {
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt = (int)learntsize_adjust_confl;
                max_learnts *= learntsize_inc;

                if (verbosity >= 1)
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n",
                           (int)conflicts,
                           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
                           (int)max_learnts, nLearnts(), (double)learnts_literals / nLearnts(), progressEstimate() * 100);
            }
        }
        else
        {
            // NO CONFLICT
            if (nof_conflicts >= 0 && conflictC >= nof_conflicts || !withinBudget())
            {
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef;
            }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if (learnts.size() - nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size())
            {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True)
                {
                    // Dummy decision level:
                    newDecisionLevel();
                }
                else if (value(p) == l_False)
                {
                    analyzeFinal(~p, conflict);
                    return l_False;
                }
                else
                {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef)
            {
                // New variable decision:
                decisions++;
                next = pickBranchLit();

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            // //M YCODE //Aqui s'ha de cambiar next per no decidir unitaries que ja sabem
            // if(unitariesPerRestart.find(toInt(~next)) != unitariesPerRestart.end() )//Si existeix com a unitaria, no deixem que decideixi
            //     next=~next;//Canviem el signe

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}

double Solver::progressEstimate() const
{
    double progress = 0;
    double F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++)
    {
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x)
{

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x + 1; seq++, size = 2 * size + 1)
        ;

    while (size - 1 != x)
    {
        size = (size - 1) >> 1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    model.clear();
    conflict.clear();
    if (!ok)
        return l_False;

    solves++;

    max_learnts = nClauses() * learntsize_factor;
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt = (int)learntsize_adjust_confl;
    lbool status = l_Undef;

    if (verbosity >= 1)
    {
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    int curr_restarts = 0;
    while (status == l_Undef)
    {
        double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        status = search(rest_base * restart_first);
        if (!withinBudget())
            break;
        curr_restarts++;
    }

    if (verbosity >= 1)
        printf("===============================================================================\n");

    if (status == l_True)
    {
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++)
            model[i] = value(i);
    }
    else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);
    return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var> &map, Var &max)
{
    if (map.size() <= x || map[x] == -1)
    {
        map.growTo(x + 1, -1);
        map[x] = max++;
    }
    return map[x];
}

void Solver::toDimacs(FILE *f, Clause &c, vec<Var> &map, Var &max)
{
    if (satisfied(c))
        return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
    fprintf(f, "0\n");
}

void Solver::toDimacs(const char *file, const vec<Lit> &assumps)
{
    FILE *f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}

void Solver::toDimacs(FILE *f, const vec<Lit> &assumps)
{
    // Handle case when solver is in contradictory state:
    if (!ok)
    {
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return;
    }

    vec<Var> map;
    Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
        {
            Clause &c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++)
    {
        assert(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "", mapVar(var(assumptions[i]), map, max) + 1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0)
        printf("Wrote %d clauses with %d variables.\n", cnt, max);
}

//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator &to)
{
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++)
        {
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher> &ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++)
    {
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);
}

void Solver::garbageCollect()
{
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size() * ClauseAllocator::Unit_Size, to.size() * ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

/*MY CODE*/

int representation2Lit(const int rep)
{
    if (rep % 2 == 0)
        return (rep / 2) + 1;
    else
        return 0 - (rep + 1) / 2;
}

int Solver::reverseRep(const int rep) const
{
    // si rep 7 (que representa -4), retorna 6 (que representa 4)
    if (rep % 2 == 0)
        return rep + 1;
    else
        return rep - 1;
}

void Solver::savePreInFile() const
{
    FILE *f = fopen("prestructure.txt", "wr");
    if (f == NULL)
    {
        fprintf(stderr, "could not open file %s\n", "prestructure.txt"), exit(1);
    }
    fprintf(f, "Capacity: %i\n", (int)preStructure.size());
    for (int ind = 0; ind < (int)preStructure.size(); ind++)
    {
        fprintf(f, "[%i] -> [", representation2Lit(ind));
        std::unordered_set<int> actualSet = preStructure.at(ind);
        std::unordered_set<int>::iterator it;
        for (it = actualSet.begin(); it != actualSet.cend(); it++)
        {
            fprintf(f, "%i,", representation2Lit((*it)));
        }
        fprintf(f, "]\n");
    }

    fclose(f);
}

void Solver::printPre() const
{
    printf("Capacity: %i\n", (int)preStructure.size());
    for (int ind = 0; ind < (int)preStructure.size(); ind++)
    {
        printf("[%i] -> [", representation2Lit(ind));
        std::unordered_set<int> actualSet = preStructure.at(ind);
        std::unordered_set<int>::iterator it;
        for (it = actualSet.begin(); it != actualSet.cend(); it++)
        {
            printf("%i,", representation2Lit((*it)));
        }
        printf("]\n");
    }
}

/*_________________________________________________________________________________________________
|
|  canAddPre : (const Lit &A, const Lit &B)  ->  [bool]
|
|  Description:
|  Pots afegir la clausula (A B) si no existeix la clausula (A -B).
|  Retorna cert si, mirant per A, veu si pots agefir B (mirara que no hi hagi -B)
_________________________________________________________________________________________________@*/
bool Solver::canAddPre(const Lit &A, const Lit &B) const
{
    return this->canAddPreDirect(toInt(~A), toInt(~B));
}
/*_________________________________________________________________________________________________
|
|  canAddPreDirect : (const int na, const int nb)  ->  [bool]
|
|  Description:
|  Direct perque no revisa que et pugis sortir de rang. Pots afegir la clausula (A B) si no existeix la clausula (A -B). Estem buscant si existeix (-A -> -B)
|  Si esta optimitzat, si na<nb, dons com es fa normalmente. Pero si nb<na, llavors de la clausula (A -B) nomes s'ha guardat (B -> A), llavors cal buscar aquesta implicacio. Notem que els valors son els negats
|  Retorna cert si, mirant per NA, veu si pots agefir B (mirara que no hi hagi -B)
_________________________________________________________________________________________________@*/
bool Solver::canAddPreDirect(const int na, const int nb) const
{
    if (optimitzar)
    {
        if (na < nb)
            return preStructure[na].find(nb) == preStructure[na].end(); // Si tenim B i no hi ha -B, llavors pots afegir. SI existeix -B, no podem afegir
        else
        {
            assert(na ^ 1 == toInt(~toLit(na)));
            assert(nb ^ 1 == toInt(~toLit(nb)));
            return preStructure[nb ^ 1].find(na ^ 1) == preStructure[nb ^ 1].end();
        }
    }
    else
        return preStructure[na].find(nb) == preStructure[na].end(); // Si tenim B i no hi ha -B, llavors pots afegir. SI existeix -B, no podem afegir
}
/*_________________________________________________________________________________________________
|
|  addPreDirect : (const Lit &A, const Lit &B)  ->  [bool]
|
|  Description:
|  S'asumeix que sabem que si es pot afegir( Pre: CanAdd(A,B) i canAdd(B,A))
|  A partir dels valors de A i B, afgeix per el literal NA el valor de B i per NB el valor de A
|  Retorna cert si hi ha agut algun canvi. Que no n'hi hagi vol dir que ja hi eren
_________________________________________________________________________________________________@*/
bool Solver::addPre(const Lit &A, const Lit &B)
{
    return addPreDirect(toInt(A), toInt(~A), toInt(B), toInt(~B));
}
/*_________________________________________________________________________________________________
|
|  addPreDirect : (const int A, const int NA, const int B, const int NB)  ->  [bool]
|
|  Description:
|  Direct perque no revisa que et pugis sortir de rang. S'asumeix que sabem que si es pot afegir
|  Afgeix per el literal NA el valor de B i per NB el valor de A
|  Retorna cert si hi ha agut algun canvi. Que no n'hi hagi vol dir que ja hi eren
_________________________________________________________________________________________________@*/
bool Solver::addPreDirect(const int A, const int NA, const int B, const int NB)
{
    int sz1, sz2;

    if (optimitzar)
    {
        if (A < B) // Dona el mateix que NA<B o A<NB
        {
            // Nomes cal guardar (NA->B)
            sz1 = (int)preStructure[NA].size();
            preStructure[NA].insert(B);
            return (int)preStructure[NA].size() > sz1;
        }
        else
        {
            // Nomes cal guardar (NB->A)
            sz2 = (int)preStructure[NB].size();
            preStructure[NB].insert(A);
            return sz2 < (int)preStructure[NB].size();
        }
    }
    else
    {
        sz1 = (int)preStructure[NA].size();
        sz2 = (int)preStructure[NB].size();
        preStructure[NA].insert(B);
        preStructure[NB].insert(A);
        return (int)preStructure[NA].size() > sz1 || sz2 < (int)preStructure[NB].size();
    }
}
/*_________________________________________________________________________________________________
|
|  satisfiedAndReduxe : (const Clause &c)  ->  [bool]
|
|  Description:
|  Retorna cert si la clausula esta satisfeta, fals altrament.
|  A mes, crea a add_tmp la clausula actual reduida (sense els asignats a false)
_________________________________________________________________________________________________@*/
bool Solver::satisfiedAndReduxe(const Clause &c)
{
    for (int i = 0; i < c.size(); i++)
    {
        // c[i] retorna un Lit
        if (value(c[i]) == l_True) // Definit com a true
            return true;
        else if (value(c[i]) == l_Undef) // Si no esta definit (sabem que no es fals)
            this->add_tmp.push(c[i]);
    }
    return false;
}
/*_________________________________________________________________________________________________
|
|  removeSatisfiedAndReduceClausules : (vec<CRef> &cs, std::vector<int> &listofthree)  ->  [bool]
|  Pre: fer_preStructure esta a true per entrar a la funcio. Mira com esta la funcio simplify() i que nomes entra a mysimplify() si fer_preStructure es true
|  Description:
|  Treu totes les clausules satisfetes.
|  Si no ho estan, mira com estan de reduides.
|  Si s'han tornat binaries, mira de detectar si provocara conflictes.
|  Si son ternaries (mida 3) afegira la posicio  a listofthree, per tal de veure despres el seu tractament
|  Retorna false si ha descobart alguna unitaria (que voldra dir que haura de tornar a començar el mysimplfy())
_________________________________________________________________________________________________@*/
bool Solver::removeSatisfiedAndReduceClausules(vec<CRef> &cs, std::vector<int> &listofthree, bool isLearnt)
{
    if (!isLearnt && !remove_satisfied) // Si remove_satisfied es fals, vol dir que s'especifica que no es busca simplificar les originals (isLearnt es fals, estem mirant les originals)
        return true;

    bool podra_continuar = true;
    Lit A, B;
    int i, j, a, b, na, nb;
    std::vector<std::pair<Lit, Lit>> listOfnewBinaries; // Ja inicialitzat
    for (i = j = 0; i < cs.size(); i++)
    {
        Clause &c = ca[cs[i]];
        this->add_tmp.clear();
        if (satisfiedAndReduxe(c))
            removeClause(cs[i]);
        else
        {
            int original_size = c.size();
            if (add_tmp.size() == 2)
            {
                if (original_size > 2) // S'ha reduit. Cal analitzar. Arribar a aquesta funcio te de pre que volem mirar les binaries, aixi que aixo ho fem si o si
                {
                    A = add_tmp[0];
                    a = toInt(A);
                    na = toInt(~A);
                    B = add_tmp[1];
                    b = toInt(B);
                    nb = toInt(~B);

                    if (not canAddPreDirect(na, nb)) // if (not canAddPre(A, B))
                    {
                        uncheckedEnqueue(A); // Hem de posar A
                        ha_simplificat_a_unitaria++;
                        removeClause(cs[i]);
                        podra_continuar = false;
                    }
                    else if (not canAddPreDirect(nb, na)) // if (not canAddPre(B, A))
                    {
                        uncheckedEnqueue(B); // Hem de posar B
                        ha_simplificat_a_unitaria++;
                        removeClause(cs[i]);
                        podra_continuar = false;
                    }
                    else
                    {
                        if (addPreDirect(a, na, b, nb)) // Si resulta que ja existeix, retorna false. Entrar aqui es que ja existeix o s'implica directament
                        {
                            noves_binaries = true;                 // Hem trobat una nova binaria
                            CRef cr = ca.alloc(add_tmp, isLearnt); // afegim la nova clausula

                            removeClause(cs[i]); // Eliminem la clausla anterior

                            cs[i] = cr; // Susbtituim la referencia per la nova reduida
                            attachClause(cr);

                            cs[j++] = cs[i]; // Avançem
                        }
                        else
                        {
                            removeClause(cs[i]); // NO ens cal la nova (ni la antiga) I no avançem. Aqui volem indicar que estem intentant posar una binaria que ja existeix o simplica
                        }
                    }
                }
                else // No s'ha reduit. Es una binaria original de mida 2. No cal analitzar aqui
                    cs[j++] = cs[i];
            }
            else if (mirar_ternaries && add_tmp.size() == 3)
            {
                if (original_size > 3) // S'ha reduit
                {
                    listOfnewBinaries.clear();

                    bool cal_eliminar_ternaria = false;

                    if (not analyze_ternaia_first_time(add_tmp, listOfnewBinaries, cal_eliminar_ternaria)) // Ha descobert unitaries.
                    {
                        podra_continuar = false;
                        removeClause(cs[i]); // Eliminem la clausula actual. Ja sabem que es satisfara per unitaria/es trobada/es
                    }
                    else // No ha trobat unitaries
                    {
                        if (listOfnewBinaries.size() > 0) // ha descobert binaries (com a maxim 3)
                        {
                            noves_binaries = true; // Noves binaries. De la llista, nomes
                            removeClause(cs[i]);   // Eliminem la clausula anterior
                            noves_binaries = true; // Noves binaries
                            for (int nc = 0; nc < (int)listOfnewBinaries.size(); nc++)
                            {
                                add_tmp.clear();                            // Com no necesitem la ternaria ja, podem utilitzar-ho ara
                                add_tmp.push(listOfnewBinaries[nc].first);  // Primer literal
                                add_tmp.push(listOfnewBinaries[nc].second); // Segon literal
                                CRef cr = ca.alloc(add_tmp, isLearnt);      // Creem la nova clausula
                                attachClause(cr);

                                if (nc == 0)    // es la primera
                                    cs[i] = cr; // Susbtituim la referencia per la nova reduida
                                else
                                    cs.push(cr);
                            }
                            cs[j++] = cs[i]; // I avançem
                        }
                        else // No ha descobert binaries
                        {
                            if (cal_eliminar_ternaria) // Voldra dir que no hem descobert noves bianries pero si que la ternaria es subsumida.
                            {
                                removeClause(cs[i]); // I no afegim la ternaria reduida ni avançar
                            }
                            else
                            {
                                CRef cr = ca.alloc(add_tmp, isLearnt); // Creem la nova clausula ternaria. Recordem que volem la reduida
                                removeClause(cs[i]);                   // Eliminem la clausla anterior
                                cs[i] = cr;                            // Susbtituim la referencia per la nova reduida
                                attachClause(cr);

                                listofthree.push_back(j); // Es la ternaria que hem de guardar. La seva posicio sera l'actual a cs
                                cs[j++] = cs[i];          // Avançem
                            }
                        }
                    }
                }
                else // Es una ternaria original
                {
                    listofthree.push_back(j);
                    cs[j++] = cs[i]; // Avançem
                }
            }
            else
                cs[j++] = cs[i]; // Avancem. Es o mirar_ternaries==true && add_tmp.size()>3 o mirar_ternaries==false && add_tmp.size()>2
        }
    }
    cs.shrink(i - j);
    return podra_continuar;
}
/*_________________________________________________________________________________________________
|
|  canbeReduceDirect : (const Lit& A, const Lit& B, const Lit& C)  ->  [bool]
|
|  Description:
|  Tenim la clausula (A B C) (en Lit) i volem veure si volem reduir a (B C)
|  Retorna cert si, mirant per el Literal A, veu si pot deduir la binaria (B C) a partir de les binaries actuals
|  Per reduir a partir de les binaries, han de existir les clausules (-A B) o (-A C). Aquestes, amb A, impliquen (A->B) i (A->C)
_________________________________________________________________________________________________@*/
bool Solver::canbeReduce(const Lit &A, const Lit &B, const Lit &C) const
{
    return canbeReduceDirect(toInt(A), toInt(B), toInt(C));
}
/*_________________________________________________________________________________________________
|
|  canbeReduceDirect : (const int NA, const int B, const int C)  ->  [bool]
|
|  Description:
|  Direct perque no revisa que et pugis sortir de rang.
|  Tenim la clausula (A B C) (en Lit) i volem veure si volem reduir a (B C)
|  Per reduir a partir de les binaries, han de existir les clausules (-A B) o (-A C). Aquestes, amb A, impliquen (A->B) i (A->C). Si trobem alguna d'aquestes implicacions, podem reduir a (B C)
|  Retorna cert si, mirant per A, veu si pot deduir la binaria (B C) a partir de les binaries actuals
_________________________________________________________________________________________________@*/
bool Solver::canbeReduceDirect(const int A, const int B, const int C) const
{
    if (optimitzar)
    {
        bool perB;
        if (A < B) // S'haura guardat (A->B) si existeix (-A B)
            perB = preStructure[A].count(B) == 1;
        else // S'haura guardat (-B->-A) si existeix (-A B)
        {
            assert(A ^ 1 == toInt(~toLit(A)));
            assert(B ^ 1 == toInt(~toLit(B)));
            perB = preStructure[B ^ 1].count(A ^ 1) == 1;
        }
        if (perB)
            return true;
        else
        {
            if (A < C) // S'haura guardat (A->C) si existeix (-A C)
                return preStructure[A].count(C) == 1;
            else // S'haura guardat (-C->-A) si existeix (-A C)
            {
                assert(C ^ 1 == toInt(~toLit(C)));
                return preStructure[C ^ 1].count(A ^ 1) == 1;
            }
        }
    }
    else
        return preStructure[A].count(B) == 1 || preStructure[A].count(C) == 1;
    // Un altre manera de fer el find(). En aquest cas, com es un set, nomes pot haver-hi 0 o 1 (si existeix) elements
}

/*_________________________________________________________________________________________________
|
|  seeThreeClausules : (vec<CRef> &cs, std::vector<int> &listofthree)  ->  [bool]
|
|  Description:
|  A partir de les llistes que indica que les clausules son ternaries, busca deduir les posibles binaries
|  que amaga a partir de les binaries que hi tenim.
|  Si descobreix alguna, elimina la ternaria perque s'enten que la binaria fa subsumcio.
|  Retorna false si ha descobart alguna unitaria (que voldra dir que haura de tornar a començar el mysimplfy())
_________________________________________________________________________________________________@*/

int Solver::seeThreeClausules(vec<CRef> &cs, std::vector<int> &listofthree, bool isLearnt)
{
    if (not mirar_ternaries) // SI no cal mirar, directament continuem
        return CAN_CONTINUE;

    if (cs.size() == 0 || listofthree.size() == 0) // No caldria, pero per si de cas
        return CAN_CONTINUE;

    if (!noves_binaries) // Si no han aparegut binaries noves, no cal tornar a fer-ho
        return CAN_CONTINUE;

    int result = CAN_CONTINUE;
    bool podra_continuar = true;
    Lit A, B, C;
    int a, b, c;
    int na, nb, nc;
    int i, j;
    std::deque<int> forats; // Es una cua
    for (i = j = 0; i < (int)listofthree.size(); i++)
    {
        bool mirarAC = true, mirarAB = true, removed = false;
        CRef nova_cr, cr = cs[listofthree[i]];
        Clause &cl = ca[cr];
        assert((int)cl.size() == 3); // Nomes per si de cas
        A = cl[0];
        a = toInt(A);
        na = toInt(~A);
        B = cl[1];
        b = toInt(B);
        nb = toInt(~B);
        C = cl[2];
        c = toInt(C);
        nc = toInt(~C);

        if (value(A) != l_Undef || value(B) != l_Undef || value(C) != l_Undef)
        // Vol dir que en algun lloc s'ha posat una Unitaria i ha agafat un valor. No fem res
        {
            // DO NOTHING
        }
        else
        {

            if (canbeReduceDirect(a, b, c)) // Mirem de trobar si existeix (B C)
            {
                ha_simplificat_a_binaria++;
                if (not canAddPreDirect(nb, nc)) // CanAddPre(B,C)
                {
                    uncheckedEnqueue(B);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = mirarAB = false;
                }
                else if (not canAddPreDirect(nc, nb)) // CanAddPre(C,B)
                {
                    uncheckedEnqueue(C);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = mirarAC = false;
                }
                else
                {
                    removeClause(cr); // Eliminem la clausula original
                    removed = true;
                    // Afegim a pre
                    if (addPreDirect(c, nc, b, nb)) // addPre(C,B) //No Existeix
                    {
                        result = NEW_BINARIES;
                        noves_binaries = true;

                        // Creem la clausla
                        add_tmp.clear();
                        add_tmp.push(B);
                        add_tmp.push(C);

                        // Afegim la nova clausula
                        nova_cr = ca.alloc(add_tmp, isLearnt);
                        attachClause(nova_cr);
                        cs[listofthree[i]] = nova_cr; // Com es la primera, sabem que hi ha lloc lliure
                    }
                    else
                    {
                        forats.push_back(listofthree[i]); // Indiquem una posicio de forat
                    }
                }
            }

            if (mirarAC && canbeReduceDirect(b, a, c)) // Mirem de trobar si existeix (A C)
            {
                ha_simplificat_a_binaria++;

                if (not canAddPreDirect(na, nc)) // CanAddPre(A,C)
                {
                    uncheckedEnqueue(A);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = mirarAB = false;
                }
                else if (not canAddPreDirect(nc, na)) // CanAddPre(C,A)
                {
                    uncheckedEnqueue(C);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = false;
                }
                else
                {
                    // Afegim a pre
                    if (addPreDirect(a, na, c, nc)) // addPre(A,C)
                    {
                        result = NEW_BINARIES;
                        // Creem la clausla
                        add_tmp.clear();
                        add_tmp.push(A);
                        add_tmp.push(C);

                        // //Afegim la nova clausula
                        nova_cr = ca.alloc(add_tmp, isLearnt);
                        attachClause(nova_cr);

                        if (not removed) // Ets la primera en remoure
                        {
                            removed = true;
                            removeClause(cr);
                            cs[listofthree[i]] = nova_cr; // Ets la primera, llavors pots ocupar la posicio
                        }
                        else // No ets la primera
                        {
                            if (forats.size() != 0) // Hi ha llocs lliures
                            {
                                int posicio_lliure = forats.front(); // Agafa el primer element de la cua
                                forats.pop_front();                  // Elimina el primer element de la cua
                                cs[posicio_lliure] = nova_cr;
                            }
                            else
                                cs.push(nova_cr); // No hi ha llocs lliures, dons al final
                        }
                    }
                    else // s'ha d'eliminar, pero no podem afegir aquesta binaria perque ja existeix
                    {
                        if (not removed)
                        {
                            removed = true;
                            removeClause(cr);
                            forats.push_back(listofthree[i]);
                        }
                        // else // NO CAL   FER RES, SI JA S'HA ELIMINAT, EL FORAT JA S'HA OMPLERT
                    }
                }
            }

            if (mirarAB && canbeReduceDirect(c, a, b)) // Mirem de trobar si existeix (A B)
            {

                ha_simplificat_a_binaria++;
                if (not canAddPreDirect(na, nb)) // CanAddPre(A,B)
                {
                    uncheckedEnqueue(A);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = false;
                }

                else if (not canAddPreDirect(nb, na)) // CanAddPre(B,A)
                {
                    uncheckedEnqueue(B);
                    ha_simplificat_a_unitaria++;
                    podra_continuar = false;
                }
                else
                {
                    // Afegim a pre
                    if (addPreDirect(a, na, b, nb)) // addPre(A,B)
                    {
                        result = NEW_BINARIES; // Creem la clausla
                        add_tmp.clear();
                        add_tmp.push(A);
                        add_tmp.push(B);

                        // //Afegim la nova clausula
                        nova_cr = ca.alloc(add_tmp, isLearnt);
                        attachClause(nova_cr);

                        if (not removed) // Ets la primera en remoure
                        {
                            removed = true;
                            removeClause(cr);
                            cs[listofthree[i]] = nova_cr; // Ets la primera, llavors pots ocupar la posicio
                        }
                        else // No ets la primera
                        {
                            if (forats.size() != 0) // Hi ha llocs lliures
                            {
                                int posicio_lliure = forats.front(); // Agafa el primer element de la cua
                                forats.pop_front();                  // Elimina el primer element de la cua
                                cs[posicio_lliure] = nova_cr;
                            }
                            else
                                cs.push(nova_cr); // No hi ha llocs lliures, dons al final
                        }
                    }
                    else // s'ha d'eliminar, pero no podem afegir aquesta binaria perque ja existeix
                    {
                        if (not removed) // si podem borrar, deixarem un forat
                        {
                            removed = true;
                            removeClause(cr);
                            forats.push_back(listofthree[i]);
                        }
                        // else // NO CAL FER RES, SI JA S'HABIA ELIMINAT, EL FORAT JA S'HA OMPLERT
                    }
                }
            }
        }
        if (not removed) // Si hem eliminat no cal que estigui a la llista de tres un altre cop
        {
            listofthree[j++] = listofthree[i];
        }
    }

    listofthree.resize(j); // El equivalent a shrink. El que fa es tallar a mida j si j es menor que la mida original

    /*Treure forats. La idea es omplir els forats amb les posicions de clausules que si existeixen i fer despres shrink. */
    // Com forats esta ordenat, podem fer aixi. SI no esta ordenat, nomes cal fer un sort abans
    int quants_forats = forats.size();
    if (quants_forats != 0)
    {
        int i;
        int posicio_buida;
        for (i = cs.size() - 1; !forats.empty() && i >= 0; i--) // Comencem per el final. Mentres quedin forats o arribem al inici (aixo es més per seguretat)
        {
            if (i == forats.back()) // Es una posicio buida
                forats.pop_back();
            else
            {
                posicio_buida = forats.front();
                forats.pop_front();
                cs[posicio_buida] = cs[i];
            }
        }
        cs.shrink(quants_forats);
    }

    if (podra_continuar)
        return result; // POT SER NEW_BINARIES O CAN_CONTINUE
    else
        return UNITARY_DISCOVERED;
}

/*_________________________________________________________________________________________________
|
|  mysimplify : [void]  ->  [bool]
|
|  Description:
|    similar al simplify(), amb la diferencia que tracta no nomes de borrar les clausules ja satisfetes,
|    sino que cerca de trobar nova informacio a partir de les noves binaries i ternaries que han aparegurt.
|    La idea es que s'apunta la informacio de totes les noves binaries i despres tracra de descobrir noves binaries
|    a partir de les ternaries.
|    Si en algun moment apareixen clausules unitaries perque detecta que hi haura conflictes si no les posa, tornara a
|    l'inici per fer propagate de nou.
|    De forma igual que simplify(), retornara cert o fals segons si ha pogut simplificar o no.
_________________________________________________________________________________________________@*/

bool Solver::mysimplify()
{
    assert(decisionLevel() == 0);
    std::vector<int> listofthreeL = std::vector<int>();
    std::vector<int> listofthreeC = std::vector<int>();

    for (;;) // Bucle infinit
    {
        if (not afegirUnitariesRestart()) // Nomes donara fals si intenta posar una unitaria quan a nivell 0 ja esta avaluada la seva contraria. Voldra dir que es unsat
            return ok = false;

        if (not insertarBinaresAC()) // Nomes donara fals si intenta posar una clausula AC que descobreis que es insatisfactible per les assignacions a nivell 0. Voldra dir que es unsat
            return ok = false;

        if (!ok || propagate() != CRef_Undef) // Si han aparegut unitaries, propaga.
            return ok = false;

        if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
            return true;

        listofthreeL.clear();
        listofthreeC.clear();

        if (removeSatisfiedAndReduceClausules(learnts, listofthreeL) &&
            removeSatisfiedAndReduceClausules(clauses, listofthreeC, false)) // Aqui descobreixes unitaries, binaries i ternaries
        {
            int result = CAN_CONTINUE;
            while (result != UNITARY_DISCOVERED) /// Notem que si un dona UNITARY_DISCOVERED, torna cap a propagate()
            {

                int result1 = CAN_CONTINUE;
                int result2 = CAN_CONTINUE;
                result = searchAllBinariImplications(); // Retorna nomes o CAN_CONTINUE, IS_UNSAT o UNITARY_DISCOVERED
                if (result == IS_UNSAT)
                    return false;
                else if (result == CAN_CONTINUE)
                {
                    result1 = seeThreeClausules(learnts, listofthreeL);
                    if (result1 != UNITARY_DISCOVERED) // En cas de descobert una unitaria, podem evitar fer aixo
                        result2 = seeThreeClausules(clauses, listofthreeC, false);

                    if (result1 == UNITARY_DISCOVERED || result2 == UNITARY_DISCOVERED) // Si s'ha descobert unitaries, sortim
                    {
                        result = UNITARY_DISCOVERED;
                    }
                    else if (result1 == NEW_BINARIES || result2 == NEW_BINARIES) // Si hem descobert alguna nova binaria, tornem a fer tot(all implications i mirar les ternaries)
                    {
                        result = NEW_BINARIES;
                    }
                    else // result1==CAN_CONTINUE && result2==CAN_CONTINUE . No hem descobert mes
                    {
                        checkGarbage();
                        rebuildOrderHeap();

                        simpDB_assigns = nAssigns();
                        simpDB_props = clauses_literals + learnts_literals; // (shouldn't depend on stats really, but it will do for now)
                        noves_binaries = false;
                        if (first_time && fer_mirar_reduccio)
                        {
                            count_simplification_B_and_T(clauses); // En un inici, aquesta es l'unica que te clausules
                            first_time = false;
                        }
                        return true; // I acabem
                    }
                }
            }
        }
    }
}

void Solver::analyze_ternaia_learnt(vec<Lit> &clearnt, int expected_backtrack)
{
    assert(clearnt.size() == 3); // Nomes si es una clausula ternaria

    Lit A, B, C;
    int a, b, c;
    int na, nb, nc;

    A = clearnt[0];
    a = toInt(A);
    na = toInt(~A); // A es l'esperada
    B = clearnt[1];
    b = toInt(B);
    nb = toInt(~B);
    C = clearnt[2];
    c = toInt(C);
    nc = toInt(~C);

    bool trobat_A = false;
    bool trobatAB, trobatAC;
    trobatAB = trobatAC = false;

    assert(canbeReduceDirect(a, b, c) == false); // No s'hauria de poder reduir a (B C)

    /*Mirem de trobar (A C)*/
    if (canbeReduceDirect(b, a, c))
    {
        in_ternaria_learnt_ha_trobat_AC++;
        trobatAC = true;
        assert(canAddPreDirect(nc, na) == true); // No s'hauria de poder reduir a (C)
        if (not canAddPreDirect(na, nc))         // CanAddPre(A,C). Si retorna false, s'ha reduit a (A)
        {
            trobat_A = true;
        }
    }
    /*Mirem de trobar (A B)*/
    if (canbeReduceDirect(c, a, b))
    {
        in_ternaria_learnt_ha_trobat_AB++;
        if (trobatAC)
            in_ternaria_learnt_ha_trobat_AC_and_AB++;
        trobatAB = true;
        assert(canAddPreDirect(nb, na) == true); // No s'hauria de poder reduir a (B)
        if (not canAddPreDirect(na, nb))         // CanAddPre(A,B). Si retorna false, s'ha reduit a (A)
        {
            trobat_A = true;
        }
    }

    CRef cr;
    int back_levelB, back_levelC;
    back_levelB = level(var(B)); // back_levelB == expected_backtrack.
    back_levelC = level(var(C)); // back_levelC<=back_levelB

    if (trobat_A) // Hem reduir a (A). Si podem reduir a (A), podem eduir tant (A B) com (A C)
    {
        in_ternaria_learnt_ha_trobat_A++;

        if (evitarRestartsExcesius && doIT(pec)) // Si decideix fer l'estrategia, es fara la quantitat de vegades indicada per el percentatge pec
        {
            unitariesPerRestart.insert(a); // Ens guardem La "unitaria" per quan fem restart
            // Ens guardem que tenim aquestes binares, pero trenquen la precondicio perque sabem que reduim a A
            // addPreDirect(a, na, b, nb);//Ens apuntem les implicacions de (A B)
            // addPreDirect(a, na, c, nc);//Ens apuntem les implicacions de (A C)

            // Funciona millor si posem la ternaria.
            cancelUntil(expected_backtrack);
            quants_cops_retrocedit_previst++;

            cr = ca.alloc(clearnt, true);
            learnts.push(cr);
            attachClause(cr);
            claBumpActivity(ca[cr]);
            uncheckedEnqueue(clearnt[0], cr);
        }
        else // Tractem la reduida com si hages sigut descoberta com a unitaria
        {
            cancelUntil(0); // Cal tornar al nivell 0
            uncheckedEnqueue(A);
        }
    }
    else if (trobatAC) // Cal retrocedir fins C
    {
        if (evitarRetroses_AC_Anterior && back_levelC < expected_backtrack && doIT(prac)) // Si decideix fer l'estrategia, es fara la quantitat de vegades indicada per el percentatge prac
        {
            assert(trobatAB == false); // No pot der que hagim trobat (A B) i volem retrocedir abans que el nivell de decisio que B.
            trobat_AC_amb_retroces_anterior++;
            quants_cops_retrocedit_previst++;
            cancelUntil(expected_backtrack);

            // Posem la ternaria
            cr = ca.alloc(clearnt, true);
            learnts.push(cr);
            attachClause(cr);
            claBumpActivity(ca[cr]);
            uncheckedEnqueue(clearnt[0], cr);
            // Recordem que hem d'afegir aquesta binaria (A C)
            afegirRecordarBinariesAC(a, c);
        }
        else // evitarRetroses_AC_Anterior==false or evitarRetroses_AC_Anterior==true and back_levelC==expected_backtrack
        {
            addPreDirect(a, na, c, nc); // Ens apuntem les implicacions de (A C)
            noves_binaries = true;
            if (back_levelC < expected_backtrack) // Hem de retrocedir abans del esperat
            {
                trobat_AC_amb_retroces_anterior++;
                quants_cops_retrocedit_abans_previst++;
            }
            else
            {
                trobat_AC_amb_retroces_esperat++;
                quants_cops_retrocedit_previst++; // En aquest cas, retrocedim igual al esperat
            }

            cancelUntil(back_levelC);

            if (trobatAB) // Si tenim (A B) amb (A C) i no hem trobat (A), vol dir que B i C son implicites entre elles (B<->C) (i que sempre es trobaran al mateix nivell)
            {
                addPreDirect(a, na, b, nb); // Ens apuntem les implicacions de (A B), per si aixo ajuda
                // quants_cops_retrocedit_previst++;//En aquest cas, retrocedim igual al esperat
                /*Creem la clausula (A B)*/
                // clearnt.clear();

                // clearnt.push(A);//Important que la primera sigui A
                // clearnt.push(B);

                // cr = ca.alloc(clearnt, true);
                // learnts.push(cr);
                // attachClause(cr);
                // claBumpActivity(ca[cr]);
            }

            /*Creem la clausula (A C)*/
            clearnt.clear();

            clearnt.push(A); // Important que la primera sigui A
            clearnt.push(C);

            cr = ca.alloc(clearnt, true);
            learnts.push(cr);
            attachClause(cr);
            claBumpActivity(ca[cr]);
            uncheckedEnqueue(clearnt[0], cr); // Anotem A
        }
    }
    else if (trobatAB)
    {
        addPreDirect(a, na, b, nb); // Ens apuntem les implicacions de (A B)
        noves_binaries = true;

        cancelUntil(back_levelB);         // Tornem fins a nivell de decisio de B
        quants_cops_retrocedit_previst++; // En aquest cas, retrocedim igual al esperat. Recordem que back_levelB == expected_backtrack.

        clearnt.clear();

        clearnt.push(A); // Important que la primera sigui A
        clearnt.push(B);

        cr = ca.alloc(clearnt, true);
        learnts.push(cr);
        attachClause(cr);
        claBumpActivity(ca[cr]);
        uncheckedEnqueue(clearnt[0], cr); // Anotem A
    }
    else // afegim la ternaria original
    {
        cancelUntil(expected_backtrack);

        cr = ca.alloc(clearnt, true);
        learnts.push(cr);
        attachClause(cr);
        claBumpActivity(ca[cr]);
        uncheckedEnqueue(clearnt[0], cr);
    }
}

bool Solver::analyze_ternaia_first_time(const vec<Lit> &clause, std::vector<std::pair<Lit, Lit>> &listOfnewBinaries, bool &cal_eliminar_ternaria)
{
    assert(clause.size() == 3); // Nomes si es una clausula ternaria

    for (int i = 0; i < clause.size(); i++)
        assert(value(clause[i]) == l_Undef);

    bool podra_continuar = true; // Per donar avis que hem afegit unitaries
    bool mirarAC = true, mirarAB = true;

    Lit A, B, C;
    int a, b, c;
    int na, nb, nc;

    A = clause[0];
    a = toInt(A);
    na = toInt(~A);
    B = clause[1];
    b = toInt(B);
    nb = toInt(~B);
    C = clause[2];
    c = toInt(C);
    nc = toInt(~C);

    if (canbeReduceDirect(a, b, c)) // Mirem de trobar si existeix (B C)
    {
        ha_simplificat_a_binaria++;
        if (not canAddPreDirect(nb, nc)) // CanAddPre(B,C)
        {
            uncheckedEnqueue(B);
            ha_simplificat_a_unitaria++;
            podra_continuar = mirarAB = false;
        }
        else if (not canAddPreDirect(nc, nb)) // CanAddPre(C,B)
        {
            uncheckedEnqueue(C);
            ha_simplificat_a_unitaria++;
            podra_continuar = mirarAC = false;
        }
        else
        {
            if (addPreDirect(c, nc, b, nb))
            {
                listOfnewBinaries.push_back(std::make_pair(B, C));
            }
            else
                cal_eliminar_ternaria = true; // Es una binaria que fa subsumsio i hem vist que ja tenim. Llavors donem avis que hem d'eliminar la ternaria
        }
    }
    if (mirarAC && canbeReduceDirect(b, a, c)) // Mirem de trobar si existeix (A C)
    {
        ha_simplificat_a_binaria++;
        if (not canAddPreDirect(na, nc)) // CanAddPre(A,C)
        {
            uncheckedEnqueue(A);
            ha_simplificat_a_unitaria++;
            podra_continuar = mirarAB = false;
        }
        else if (not canAddPreDirect(nc, na)) // CanAddPre(C,B)
        {
            uncheckedEnqueue(C);
            ha_simplificat_a_unitaria++;
            podra_continuar = mirarAC = false;
        }
        else
        {
            if (addPreDirect(c, nc, a, na))
            {
                listOfnewBinaries.push_back(std::make_pair(A, C));
            }
            else
                cal_eliminar_ternaria = true; // Es una binaria que fa subsumsio i hem vist que ja tenim. Llavors donem avis que hem d'eliminar la ternaria
        }
    }
    if (mirarAB && canbeReduceDirect(c, a, b)) // Mirem de trobar si existeix (A B)
    {
        ha_simplificat_a_binaria++;
        if (not canAddPreDirect(nb, na)) // CanAddPre(B,A)
        {
            uncheckedEnqueue(B);
            ha_simplificat_a_unitaria++;
            podra_continuar = false;
        }
        else if (not canAddPreDirect(na, nb)) // CanAddPre(A,B)
        {
            uncheckedEnqueue(A);
            ha_simplificat_a_unitaria++;
            podra_continuar = false;
        }
        else
        {
            if (addPreDirect(a, na, b, nb))
            {
                listOfnewBinaries.push_back(std::make_pair(A, B));
            }
            else
                cal_eliminar_ternaria = true; // Es una binaria que fa subsumsio i hem vist que ja tenim. Llavors donem avis que hem d'eliminar la ternaria
        }
    }

    return podra_continuar;
}

int Solver::searchAllBinariImplications()
{

    if (not mirar_All_implications)
        return CAN_CONTINUE;

    if (not noves_binaries)
        return CAN_CONTINUE;

    if (contador_quants_cops_cerca_implicacio < 0)
        return CAN_CONTINUE;

    if (contador_quants_cops_cerca_implicacio > 0)
    {
        contador_quants_cops_cerca_implicacio--;
        return CAN_CONTINUE;
    }

    _quants_cops_cerca_fet++;
    double initial_time = cpuTime();
    double final_time;
    int response = CAN_CONTINUE;

    const int quants_literals = preStructure.size(); // Aqui ens apuntem quants literals tenim

    std::vector<bool> literals_prohibits = std::vector<bool>(quants_literals, false); // Aqui apuntem si descobrim literals prohibits. Detall, sembla ser que un vector<bool> a c++ esta optimitzat en memoria (cada posicio ocupa un bit)
    std::deque<int> actualImplications;
    std::unordered_set<int> noves_unitaries; // Pot ser una altre std
    bool lit_prohibit;                       // Per indicar si hem arribat a una contradiccio

    std::vector<std::unordered_set<int>> estructura_implicacions_optimitzada; // Per quan es fa i esta optimitzat, aqui es guardara la info
    std::unordered_set<int> implicacions_actual_opt;                          // Ja s'ha inicialitzat

    if (optimitzar) // SI esta optimitzat, cal tornar a afegir tota la info a totes les implicacions*/
    {
        estructura_implicacions_optimitzada.resize(quants_literals, std::unordered_set<int>()); // Ja s'inicialitza. Aqui li assignem la mida

        for (int actual_literal = 0; actual_literal < quants_literals; actual_literal++)
        {
            const Lit actual = toLit(actual_literal); // Actual, pero en format de Lit
            if (value(actual) != l_Undef)
                preStructure[actual_literal].clear(); // Si esta definit, ja no importa les implicacions que te. Fem espai
            else
            {
                Lit _lit;
                for (auto it = preStructure[actual_literal].begin(); it != preStructure[actual_literal].end();) // Millor amb un iterador explicit. Ens permet fer aixo amb erase
                {
                    int literal = (*it);
                    _lit = toLit(literal);
                    if (value(_lit) == l_Undef)
                    {
                        // Tenim (actual -> _lit). Hem de afegir (not _lit -> not actual)
                        preStructure[toInt(~_lit)].insert(toInt(~actual));
                        ++it;
                    }
                    else // Nomes podra ser True si actual no esta definit. Qualsevol ja definit millor els borrem
                    {
                        it = preStructure[actual_literal].erase(it); // Retorna el seguent iterador valid. eliminem el valor apuntat per it
                    }
                }
            }
        }
    }

    for (int actual_literal = 0; actual_literal < quants_literals; actual_literal++) // Cada posicio es un literal. Hem de mirar tots
    {
        const Lit actual = toLit(actual_literal);         // Actual, pero en format de Lit
        const Lit actual_negat = ~actual;                 // El seu negat
        const int int_actual_negat = toInt(actual_negat); // int del Lit negat

        actualImplications.clear(); // Netejem Per si de cas. No posar aixo provocava errors

        implicacions_actual_opt.clear(); // Si utilitzem swap(), no cal. Swap ja s'encarrega de deixar-ho buit perque estem intercanviant per set buits

        if (value(actual) == l_Undef) // Si no es true o false. Basicament, no tenim definit el literal
        {
            lit_prohibit = false;
            Lit _lit;
            for (int literal : preStructure[actual_literal]) // Per cada int (que representa cada literal) de la posicio de preStructure de actual
            {
                if (literals_prohibits[literal]) // Podria ser que hem descobert ara que esta prohibit. Com sabem que porta a una que esta prohibit, doncs actual tampoc pot ser
                {
                    lit_prohibit = true; // Marquem com que actual no pot ser
                    break;               // Sortim del for
                }
                _lit = toLit(literal);
                if (value(_lit) == l_Undef)                // No pot ser l_False perque llavors no tindriem actual com undef. Com a minim pot ser que si ja esta definit com a true, llavors ignorem
                    actualImplications.push_back(literal); // Ens apuntem a la cua quin hem de mirar
            }

            if (lit_prohibit) // Hem descobert que actual esta prohibit
            {
                if (literals_prohibits[int_actual_negat]) // Aqui hem descobert que el negat de actual tambe esta prohibit. Si els dos literals estan prohibits, el 2SAt no es pot satisfer, llavors tot el problema es UNSAT
                {
                    printf("-----------------HEM descobert que es UNSAT en 2SAT--------------------\n");
                    final_time = cpuTime();
                    temps_total_fet_cerca += final_time - initial_time;

                    return IS_UNSAT;
                }

                noves_unitaries.insert(int_actual_negat); // Com actual esta prohibit, Hem d'assegurar de posar el negat.

                literals_prohibits[actual_literal] = true; // Si es posa obligatoriament el negat, llavors el actual esta prohibit
                response = UNITARY_DISCOVERED;
            }
            else
            {
                int lit_a_mirar;
                std::vector<bool> mirats = std::vector<bool>(quants_literals, false);

                while (not actualImplications.empty()) // Mentre no ens quedem sense
                {
                    lit_a_mirar = actualImplications.front();
                    _lit = toLit(lit_a_mirar);

                    if (lit_a_mirar != actual_literal && not mirats[lit_a_mirar]) // Si no l'hem mirat abans. Pot ser que ens trobem alguns que impliquin l'actual. En aquests casos ignorem
                    {
                        bool existeix_el_seu_contrari_a_actual;
                        if (optimitzar)
                        {
                            existeix_el_seu_contrari_a_actual = implicacions_actual_opt.count(toInt(~_lit)) != 0;
                        }
                        else
                            existeix_el_seu_contrari_a_actual = preStructure[actual_literal].count(toInt(~_lit)) != 0;

                        if (literals_prohibits[lit_a_mirar] || toInt(actual_negat) == lit_a_mirar || existeix_el_seu_contrari_a_actual)
                        // Hem descobert que aquest es un literal prohibit, hem arribat al negat de actual o existeix el seu negat a les implicacions
                        {
                            lit_prohibit = true;
                            break; // Sortim del while
                        }

                        for (int int_literal_implicat : preStructure[lit_a_mirar]) // Agafem tots els que lit_a_mirar impliquen
                        {
                            Lit literal_implicat = toLit(int_literal_implicat);
                            if (!mirats[int_literal_implicat] && value(literal_implicat) == l_Undef && actual_literal != int_literal_implicat)
                            {
                                bool esta_prohibit = literals_prohibits[int_literal_implicat];
                                bool es_el_negat_de_actual = int_actual_negat == int_literal_implicat;

                                if (optimitzar)
                                    existeix_el_seu_contrari_a_actual = implicacions_actual_opt.find(toInt(~literal_implicat)) != implicacions_actual_opt.end();
                                else
                                    existeix_el_seu_contrari_a_actual = preStructure[actual_literal].find(toInt(~literal_implicat)) != preStructure[actual_literal].end();
                                if (esta_prohibit || es_el_negat_de_actual || existeix_el_seu_contrari_a_actual)
                                {
                                    lit_prohibit = true;
                                    break; // Sortim del for
                                }

                                if (!lit_prohibit)
                                {
                                    actualImplications.push_back(int_literal_implicat);
                                }
                            }
                        }
                        if (lit_prohibit)
                        {
                            break; // Sortim del while
                        }
                        // else
                        _lit = toLit(lit_a_mirar);
                        assert(actual_literal != toInt(~_lit));
                        assert(actual_literal != toInt(_lit));
                        assert(preStructure[actual_literal].find(toInt(~_lit)) == preStructure[actual_literal].end());
                        assert(implicacions_actual_opt.find(toInt(~_lit)) == implicacions_actual_opt.end());
                        if (optimitzar)
                        {
                            implicacions_actual_opt.insert(lit_a_mirar); // Potser que es aqui podem mirar si cal inserir o no amb optimitzar. TODO: mirar-ho
                        }
                        else
                            preStructure[actual_literal].insert(lit_a_mirar); // Inserim a les implicacions. Arribat aqui, sabem que es pot inserir.
                        mirats[lit_a_mirar] = true;
                    }

                    actualImplications.pop_front();
                }

                if (lit_prohibit) // Hem descobert que actual esta prohibit
                {
                    if (literals_prohibits[int_actual_negat]) // Aqui hem descobert que el negat de actual tambe esta prohibit. Si els dos literals estan prohibits, el 2SAt no es pot satisfer, llavors tot el problema es UNSAT
                    {
                        printf("-----------------Hem descobert que es UNSAT en 2SAT--------------------\n");
                        final_time = cpuTime();
                        temps_total_fet_cerca += final_time - initial_time;

                        return IS_UNSAT;
                    }

                    // if( value(actual_negat) == l_Undef )
                    noves_unitaries.insert(int_actual_negat);  // Com actual esta prohibit, Hem d'assegurar de posar el negat. Detall, aixo fara que, si el seguent sera mirar el negat, no entri perque ja no estara undef (crec)
                    literals_prohibits[actual_literal] = true; // Si es posa obligatoriament el negat, llavors el actual esta prohibit
                    response = UNITARY_DISCOVERED;

                    preStructure[actual_literal].clear(); // Ja no importa el que impliquen. Netejem per donar espai
                }
                else if (optimitzar) // Un cop arribat aqui, vol dir que podem afegir les implicacions, pero cal optimitzarles
                {
                    // Primer eliminem totes les que siguin < que actual
                    for (auto it = implicacions_actual_opt.begin(); it != implicacions_actual_opt.end();) // Millor amb un iterador explicit. Ens permet fer aixo amb erase
                    {
                        int literal = (*it);
                        if (literal < actual_literal)
                        {
                            it = implicacions_actual_opt.erase(it); // Retorna el seguent iterador valid. eliminem el valor apuntat per it
                        }
                        else
                            it++;
                    }

                    estructura_implicacions_optimitzada[actual_literal].swap(implicacions_actual_opt); // Li donem els valors i implicacions_actual_opt queda buida perque esta inicialitzat buit
                }
                else
                {
                    // No cal fer res
                }
            }
        }
        else if (value(actual) == l_True) // Si actual esta definit com a true, llavors el prhibit es el negat
        {
            literals_prohibits[int_actual_negat] = true; // Negat esta prohibit
            preStructure[actual_literal].clear();        // Ja no importa el que impliquen. Netejem per donar espai
        }
        else
        {
            preStructure[actual_literal].clear();      // Ja no importa el que impliquen. Netejem per donar espai
            literals_prohibits[actual_literal] = true; // Actual esta prohibit
        }
    }

    for (int int_u : noves_unitaries) // Una manera de fer un for amb unordered_set
    {
        // printf("Hem descobert una unitaria\n");
        Lit u = toLit(int_u);
        assert(value(u) == l_Undef);   // Tinc la sensacio que aquest assert no cal fer
        unitaries_descobertes_cerca++; // Cadascun es un descobert. Incrementem el contador aqui
        uncheckedEnqueue(u);
        response = UNITARY_DISCOVERED; // No cal, pero per si de cas
    }
    // printf("Ja he acabat de buscar implicacions\n");

    if (optimitzar)
    {
        preStructure.swap(estructura_implicacions_optimitzada); // Crec que es igual d'eficient que move()
    }

    contador_quants_cops_cerca_implicacio = MAX_COPS;

    final_time = cpuTime();
    temps_total_fet_cerca += final_time - initial_time;

    return response; // Aqui nomes es o UNITARY_DISCOVERED o CAN_CONTINUE
}

void Solver::count_simplification_B_and_T(vec<CRef> &cs)
{
    assert(first_time == true); // Un cop cridat per primer cop, no hauria de tornarse a cridar
    if(!fer_mirar_reduccio)//Per si de cas. Es un retorn buit, ja que la funcio es un void. Aixi sortim de la funcio rapidament
        return ;
    int nB, nT;                 // NBinaries i nTernaries
    nB = nT = 0;
    for (int i = 0; i < cs.size(); i++)
    {
        Clause &c = ca[cs[i]];
        if (c.size() == 2)
            nB++;
        else if (c.size() == 3)
            nT++;
    }

    std::ofstream arxiu("reduccio.txt");

    if (arxiu.is_open())
    {
        arxiu << "Binaries Originals:      " << original_binaries_count << "\n";
        arxiu << "Ternaries Originals:     " << original_ternaries_count << "\n";
        arxiu << "Binaries Despres Simplificar:  " << nB << "\n";
        arxiu << "Ternaries Despres Simplificades: " << nT << "\n";
        arxiu.close();
    }
}

bool Solver::afegirUnitariesRestart()
{
    assert(decisionLevel() == 0);
    Lit A;
    int a;
    for (auto it = unitariesPerRestart.begin(); it != unitariesPerRestart.end();)
    {
        a = *it;
        A = toLit(a);
        if (value(A) == l_Undef)
        {
            unitariesDescobertesAfegidesRestart++;
            uncheckedEnqueue(A); // Afegim
        }
        else if (value(A) == l_True)
        {
            unitariesRedescobertes++;
        }
        else
            return false; // Vol dir que volem afegir a nivell 0 un literal a cert quan ja esta evaluat a fals. Vol dir que es unsat

        it = unitariesPerRestart.erase(it); // Eliminar i avancem iterador
    }
    return true;
}

void Solver::afegirRecordarBinariesAC(int C, int A)
// Quan descobrim AC pero no volem retrocedir, ens apuntem que volem recordar que existeix la clausula (A C).
{
    if (evitarRetroses_AC_Anterior)
    {
        std::pair<int, int> clausula;

        if (A < C)
            clausula = std::make_pair(A, C);
        else
            clausula = std::make_pair(C, A);

        binaries_AC_Apreses.insert(clausula);
    }
}
bool Solver::insertarBinaresAC()
// La idea es, quan el trail retrocedeix a nivell de decisio 0, en aquest moment inserim la clausula (A C), que abans no podiem. Si detectem que alguna clausula es unsat, tota la formula es unsat
{
    if (evitarRetroses_AC_Anterior) // Nomes cal si aixo esta activat
    {
        assert(decisionLevel() == 0);
        for (const auto &clausula : binaries_AC_Apreses)
        {
            Lit A = toLit(clausula.first);
            Lit C = toLit(clausula.second);

            if (value(A) == l_True || value(C) == l_True) // Si alguna esta satisfeta, ignorem la clausula
            {
                // Do Nothing
            }
            else if (value(A) == l_False) // Esta avaluat a false, implica que la clausula es (C). Cal afegir C si o si
            {
                if (value(C) == l_Undef)
                {
                    uncheckedEnqueue(C);
                }
                else
                    return false; // Sabem que no esta a true perque aixo ho hem mirat abans. Si esta a false, es una clausula buida a nivell 0
            }
            else if (value(C) == l_False) // Esta avaluat a false, implica que la clausula es (A). Cal afegir A si o si
            {
                if (value(A) == l_Undef)
                {
                    uncheckedEnqueue(A);
                }
                else
                    return false; // Sabem que no esta a true perque aixo ho hem mirat abans. Si esta a false, es una clausula buida a nivell 0
            }
            else // Els dos son undef. Podem afegir la clausula
            {
                if (not canAddPre(A, C))
                {
                    uncheckedEnqueue(A);
                }
                else if (not canAddPre(C, A))
                {
                    uncheckedEnqueue(C);
                }
                if (addPre(A, C)) // Si no ha crescut, es que ja hi havia la binaria
                {
                    this->add_tmp.clear(); // Aprofitem aquesta per incloure la binaria
                    this->add_tmp.push(A);
                    this->add_tmp.push(C);

                    CRef cr = ca.alloc(add_tmp, false);
                    clauses.push(cr);
                    attachClause(cr);
                    noves_binaries = true;
                    clausulesAC_afegides++;
                }
                else
                {
                    clausulesAC_reedescobertes++;
                }
            }
        }
        binaries_AC_Apreses.clear(); // Buidem
    }
    return true; // Tot ha anat be
}

void Solver::printinfo(double cpu_time) const
{
    printf("\n");
    printf("Temps_cpu_resoldre(cpuTime_sense_temps_cerca):      %.5f\n", cpu_time - temps_total_fet_cerca);

    printf("\n");
    printf("Unitaries_learnt:                                    %i\n", ha_aparegut_unitaria_learnt);

    printf("\n");
    printf("Binaries_learnt:                                    %i\n", ha_aparegut_binaria_learnt);
    printf("Binaries_learnt_nova:                               %i\n", ha_apres_binaria_learnt_nova);
    printf("Binaries_learnt_existent:                           %i\n", ha_apres_binaria_learnt_ja_existent);
    printf("Unitaries_A_in_binaries_learnt_trobada:             %i\n", in_binaria_learnt_ha_trobat_A);

    printf("\n");
    printf("Ternaries_learnt:                                    %i\n", ha_aparegut_ternaria_learnt);
    printf("Binaria_AC_trobada:                                  %i\n", in_ternaria_learnt_ha_trobat_AC);
    printf("Binaria_AC_amb_retroces_esperat:                     %i\n", trobat_AC_amb_retroces_esperat);
    printf("Binaria_AC_amb_retroces_anterior:                    %i\n", trobat_AC_amb_retroces_anterior);
    printf("Binaria_AB_trobada:                                  %i\n", in_ternaria_learnt_ha_trobat_AB);
    printf("Binaries_AC_&_AB_trobades:                           %i\n", in_ternaria_learnt_ha_trobat_AC_and_AB);
    printf("Unitaria_A_in_ternaria_learnt_trobada:               %i\n", in_ternaria_learnt_ha_trobat_A);

    printf("\n");
    printf("Retroces_per_reduccio_a_binaries_esperat:              %i\n", quants_cops_retrocedit_previst);
    printf("Retroces_per_reduccio_a_binaries_anterior_al_esperat:  %i\n", quants_cops_retrocedit_abans_previst);

    printf("\n");
    printf("Simplificat_a_Binaries(NO CDCL):                    %i\n", ha_simplificat_a_binaria);
    printf("Simplificat_a_Unitaries(NO CDCL):                   %i\n", ha_simplificat_a_unitaria);

    printf("\n");
    printf("Quants_cops_fet_cerca_implicacions:                 %i\n", _quants_cops_cerca_fet);
    printf("Temps_total_segons_fet_cerca_implicacions:          %.5f\n", temps_total_fet_cerca);
    printf("Temps_mitjana_segons_fet_cerca_implicacions:        %.5f\n",
           _quants_cops_cerca_fet == 0 ? 0.0 : (double)temps_total_fet_cerca / _quants_cops_cerca_fet);
    printf("Unitaries_descobertes_per_la_cerca_de_implicacions: %i\n", unitaries_descobertes_cerca);

    printf("\n");
    printf("Unitaries_reedescobertes_en_restart:                  %i\n", unitariesRedescobertes);
    printf("Unitaries_evitat_restart_afegits:                     %i\n", unitariesDescobertesAfegidesRestart);

    printf("\n");
    printf("Binaries_AC_recordades_afegides:                  %i\n", clausulesAC_afegides);
    printf("Binaries_AC_recordades_reedescobertes:            %i\n", clausulesAC_reedescobertes);
}

void Solver::inicialitzePreStructure(const int sizeOfVariables)
{

    if (fer_preStructure) // Per estalviar temps, nomes cal que inicialitzi preStructure si ho volem fer
    {
        assert(sizeOfVariables != 0);
        int n = (int)abs(sizeOfVariables);
        preStructure.resize(n * 2, std::unordered_set<int>());
    }
}

bool Solver::doIT(const int percentatge)
{
    if (percentatge <= 0)
        return false;
    if (percentatge >= 100)
        return true;

    return irand(seed_for_doit, 100) <= percentatge;
}