import React, { createContext, useContext, useState, useEffect } from 'react';

const ScenarioContext = createContext();

export const useScenario = () => useContext(ScenarioContext);

export const ScenarioProvider = ({ children }) => {
  const [currentScenario, setCurrentScenario] = useState('baseline');
  const [scenarios, setScenarios] = useState([]);
  const [kpis, setKpis] = useState({
    demands: 0,
    planned: 0,
    leftovers: 0,
    projects: 0,
    wbs: 0,
    revenue: 0,
  });
  const [isAutopilot, setIsAutopilot] = useState(true);
  const [isLoading, setIsLoading] = useState(false);
  const [selectedPartCode, setSelectedPartCode] = useState(null);
  const [selectedDemandId, setSelectedDemandId] = useState(null);
  const [activeTab, setActiveTab] = useState('mps');

  // Fetch all scenarios
  const fetchScenarios = async () => {
    try {
      const res = await fetch('/api/scenarios');
      const data = await res.json();
      setScenarios(data);
    } catch (err) {
      console.error('Error fetching scenarios:', err);
    }
  };

  // Fetch live KPIs
  const fetchKpis = async () => {
    try {
      const res = await fetch('/api/kpis');
      const data = await res.json();
      setKpis(data);
    } catch (err) {
      console.error('Error fetching KPIs:', err);
    }
  };

  // Switch scenario
  const switchScenario = async (scenarioCode) => {
    setIsLoading(true);
    try {
      const res = await fetch('/api/scenarios/switch', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ scenario_code: scenarioCode }),
      });
      const data = await res.json();
      if (data.status === 'success') {
        setCurrentScenario(scenarioCode);
        await fetchKpis();
      } else {
        alert(`切换场景失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error switching scenario:', err);
    } finally {
      setIsLoading(false);
    }
  };

  // Create scenario
  const createScenario = async (code, name) => {
    setIsLoading(true);
    try {
      const res = await fetch('/api/scenarios/create', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ scenario_code: code, scenario_name: name }),
      });
      const data = await res.json();
      if (data.status === 'success') {
        await fetchScenarios();
        await switchScenario(code);
      } else {
        alert(`创建沙箱失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error creating scenario:', err);
    } finally {
      setIsLoading(false);
    }
  };

  // Delete scenario
  const deleteScenario = async (code) => {
    if (code === 'baseline') return;
    setIsLoading(true);
    try {
      const res = await fetch('/api/scenarios/delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ scenario_code: code }),
      });
      const data = await res.json();
      if (data.status === 'success') {
        await fetchScenarios();
        if (currentScenario === code) {
          setCurrentScenario('baseline');
        }
        await fetchKpis();
      } else {
        alert(`删除沙箱失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error deleting scenario:', err);
    } finally {
      setIsLoading(false);
    }
  };

  // Merge scenario to production (baseline)
  const mergeScenario = async (code) => {
    if (code === 'baseline') return;
    setIsLoading(true);
    try {
      const res = await fetch('/api/scenarios/merge', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ scenario_code: code }),
      });
      const data = await res.json();
      if (data.status === 'success') {
        setCurrentScenario('baseline');
        await fetchScenarios();
        await fetchKpis();
        alert('成功将沙箱数据合并到生产主计划 (Baseline)！');
      } else {
        alert(`合并沙箱失败: ${data.message}`);
      }
    } catch (err) {
      console.error('Error merging scenario:', err);
    } finally {
      setIsLoading(false);
    }
  };

  // Trigger C++ planning engine simulation cycle
  const runSimulation = async () => {
    setIsLoading(true);
    try {
      const res = await fetch('/api/run');
      const data = await res.json();
      await fetchKpis();
      return data;
    } catch (err) {
      console.error('Error running simulation:', err);
      return { status: 'error', message: err.message };
    } finally {
      setIsLoading(false);
    }
  };

  useEffect(() => {
    fetchScenarios();
    fetchKpis();
  }, []);

  return (
    <ScenarioContext.Provider
      value={{
        currentScenario,
        scenarios,
        kpis,
        isAutopilot,
        setIsAutopilot,
        isLoading,
        fetchScenarios,
        fetchKpis,
        switchScenario,
        createScenario,
        deleteScenario,
        mergeScenario,
        runSimulation,
        activeTab,
        setActiveTab,
        selectedPartCode,
        setSelectedPartCode,
        selectedDemandId,
        setSelectedDemandId,
      }}
    >
      {children}
    </ScenarioContext.Provider>
  );
};
