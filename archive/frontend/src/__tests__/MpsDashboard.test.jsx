import React from 'react';
import { render, screen } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import MpsDashboard from '../views/MPS/MpsDashboard';

// Mock the useScenario context hook
vi.mock('../contexts/ScenarioContext', () => ({
  useScenario: () => ({
    currentScenario: 'baseline',
    fetchKpis: vi.fn(),
    runSimulation: vi.fn(),
    setSelectedPartCode: vi.fn(),
    setActiveTab: vi.fn()
  })
}));

describe('MpsDashboard Component Tests', () => {
  it('renders finished goods search panel correctly', () => {
    render(<MpsDashboard />);
    
    // Verify search input is rendered
    const searchInput = screen.getByPlaceholderText(/搜索成品零件号/i);
    expect(searchInput).toBeDefined();
  });

  it('renders layout grid headers', () => {
    render(<MpsDashboard />);
    
    const gridHeader = screen.getByText(/时序平衡排产网格/i);
    expect(gridHeader).toBeDefined();
  });
});
